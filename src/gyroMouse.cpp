#include "gyroMouse.h"

#include <Arduino.h>
#include <math.h>

namespace {

// 每轴：3 点中值 → 限斜率 → EMA
struct AxisFilter {
    float   buf[3]    = {0.0f, 0.0f, 0.0f};
    uint8_t idx       = 0;
    float   prevMed   = 0.0f;
    float   filt      = 0.0f;

    static float median3(float a, float b, float c) {
        if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
        if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
        return c;
    }

    float update(float raw) {
        buf[idx] = raw;
        idx = (idx + 1) % 3;

        // 中值滤波：干掉孤立单帧脉冲
        float med = median3(buf[0], buf[1], buf[2]);

        // 限斜率：削掉 1~2 帧的机械冲击，但持续运动照样跟得上
        float d = med - prevMed;
        if (d > GM_SLEW_LIMIT)       med = prevMed + GM_SLEW_LIMIT;
        else if (d < -GM_SLEW_LIMIT) med = prevMed - GM_SLEW_LIMIT;
        prevMed = med;

        // EMA 低通
        filt += GM_EMA_ALPHA * (med - filt);
        return filt;
    }

    void reset() {
        buf[0] = buf[1] = buf[2] = 0.0f;
        idx     = 0;
        prevMed = 0.0f;
        filt    = 0.0f;
    }
};

AxisFilter s_fx;
AxisFilter s_fy;

bool     s_engaged    = false;   // 迟滞死区状态
float    s_accX       = 0.0f;    // 亚像素累加器
float    s_accY       = 0.0f;
uint32_t s_lockUntil  = 0;
bool     s_btnDown    = false;
uint8_t  s_dragFrames = 0;       // 锁定期间连续超门限的帧数

inline bool lockActive() {
    return (int32_t)(millis() - s_lockUntil) < 0;   // millis 回绕安全
}

inline float softDeadzone(float v) {
    float a = fabsf(v);
    if (a <= GM_DEADZONE_EXIT) return 0.0f;
    a -= GM_DEADZONE_EXIT;                          // 减掉停止阈值，起步不突跳
    return (v < 0.0f) ? -a : a;
}

}  // namespace

namespace gyroMouse {

void reset() {
    s_fx.reset();
    s_fy.reset();
    s_engaged    = false;
    s_accX       = 0.0f;
    s_accY       = 0.0f;
    s_lockUntil  = 0;
    s_btnDown    = false;
    s_dragFrames = 0;
}

void lock(uint32_t ms) {
    s_lockUntil  = millis() + ms;
    s_dragFrames = 0;
    s_accX       = 0.0f;
    s_accY       = 0.0f;
    s_engaged    = false;
}

bool locked() {
    return lockActive();
}

void setButtonDown(bool down) {
    s_btnDown = down;
}

void update(float rawX, float rawY, int sensitivity, int &dx, int &dy) {
    dx = 0;
    dy = 0;

    // 冻结期间仍然喂滤波器，让它收敛；只是不上报位移
    float fx = s_fx.update(rawX);
    float fy = s_fy.update(rawY);

    if (lockActive()) {
        // 持续运动 → 用户真的要拖拽，提前解锁，别让手感发木
        if (fabsf(fx) > GM_DRAG_GATE || fabsf(fy) > GM_DRAG_GATE) {
            if (s_dragFrames < 255) s_dragFrames++;
        } else {
            s_dragFrames = 0;
        }

        if (s_dragFrames < GM_DRAG_FRAMES) {
            s_accX    = 0.0f;
            s_accY    = 0.0f;
            s_engaged = false;
            return;
        }
        s_lockUntil = 0;   // 确认是拖拽，立即放行
    }

    // 严格模式：按住按键期间完全冻结
    if (GM_FREEZE_WHILE_DOWN && s_btnDown) {
        s_accX    = 0.0f;
        s_accY    = 0.0f;
        s_engaged = false;
        return;
    }

    // 迟滞死区：起步用 ENTER，停下用 EXIT，避免在阈值边缘来回抖
    float ax = fabsf(fx);
    float ay = fabsf(fy);
    if (!s_engaged) {
        if (ax > GM_DEADZONE_ENTER || ay > GM_DEADZONE_ENTER) s_engaged = true;
    } else {
        if (ax < GM_DEADZONE_EXIT && ay < GM_DEADZONE_EXIT) s_engaged = false;
    }
    if (!s_engaged) {
        s_accX = 0.0f;
        s_accY = 0.0f;
        return;
    }

    // 亚像素累加：小数位移留着，够 1px 才发，慢速移动不丢
    float sens = (float)sensitivity * 0.05f;
    s_accX += -softDeadzone(fx) * sens;
    s_accY += -softDeadzone(fy) * sens;

    dx = (int)s_accX;
    dy = (int)s_accY;
    s_accX -= (float)dx;
    s_accY -= (float)dy;

    // HID 位移字段是 int8，必须限幅，否则大甩动会溢出反向
    dx = constrain(dx, -127, 127);
    dy = constrain(dy, -127, 127);
}

}  // namespace gyroMouse
