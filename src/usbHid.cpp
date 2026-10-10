#include "usbHid.h"
#include "gyroMouse.h"

USBHIDMouse mouse;
USBHIDKeyboard keyboard;

void handleUsbMode(bool mouseMode, bool gyroMode) {
    if (mouseMode) {
        usbMouse(gyroMode);
    } else  {
        usbKeyboard();
    }
    delay(5);
}

void usbMouse(bool gyroMode) {
    static bool mouseInited = false;
    if (!mouseInited) { mouse.begin(); mouseInited = true; }

    // 切换陀螺仪/方向键模式时清掉滤波状态与按键沿状态，避免残留
    static bool wasGyro = false;
    static bool prevLeft = false, prevRight = false;
    if (gyroMode != wasGyro) {
        gyroMouse::reset();
        prevLeft = prevRight = false;
        wasGyro = gyroMode;
    }

    int moveX = 0;
    int moveY = 0;
    int wheel = 0;

    // Scroll wheel: ; = up, . = down — 50ms cooldown
    static unsigned long lastWheelTime = 0;
    if (millis() - lastWheelTime > 50) {
        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            wheel = 1;
            lastWheelTime = millis();
        } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            wheel = -1;
            lastWheelTime = millis();
        }
    }

    if (gyroMode) {
        // ── 先处理按键：按下瞬间会带动整机震动 ──
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        bool left  = status.enter;
        bool right = M5Cardputer.Keyboard.isKeyPressed('\\');

        // 按下沿 → 锁定光标，避免「点击」被识别成「拖拽」
        if ((left && !prevLeft) || (right && !prevRight)) {
            gyroMouse::lock();
        }
        prevLeft  = left;
        prevRight = right;
        gyroMouse::setButtonDown(left || right);

        if (left)  mouse.press(MOUSE_BUTTON_LEFT);  else mouse.release(MOUSE_BUTTON_LEFT);
        if (right) mouse.press(MOUSE_BUTTON_RIGHT); else mouse.release(MOUSE_BUTTON_RIGHT);

        // ── 再算位移：中值→限斜率→EMA→迟滞死区→亚像素累加 ──
        float rawX = gyroZ * cos(tiltAngle) + gyroY * sin(tiltAngle);
        float rawY = gyroX;
        int dx = 0, dy = 0;
        gyroMouse::update(rawX, rawY, mouseSensitivity, dx, dy);

        mouse.move((int8_t)dx, (int8_t)dy, (int8_t)wheel);
        return;
    }

    // Original arrow-key mouse control
    if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        float spd = (mouseSensitivity + 1) / 2;

        if (M5Cardputer.Keyboard.isKeyPressed('/')) {
            moveX = (int)spd;
        } 
        
        if (M5Cardputer.Keyboard.isKeyPressed(',')) {
            moveX = -(int)spd;
        }

        // clics souris
        if (status.enter) {
            mouse.press(MOUSE_BUTTON_LEFT);
        } else if (M5Cardputer.Keyboard.isKeyPressed('\\')) {
            mouse.press(MOUSE_BUTTON_RIGHT);
        }
        // Send
        moveX = constrain(moveX, -127, 127);
        moveY = constrain(moveY, -127, 127);
        mouse.move(moveX, moveY, wheel);

    } else {
        mouse.release(MOUSE_BUTTON_LEFT);
        mouse.release(MOUSE_BUTTON_RIGHT);
    }
}

void usbKeyboard() {
    static bool inited = false;
    if (!inited) { keyboard.begin(); inited = true; }

    if (!M5Cardputer.Keyboard.isChange()) return;

    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    KeyReport report = {0};
    report.modifiers = status.modifiers;

    uint8_t idx = 0;
    for (auto k : status.hid_keys) {
        if (idx < 6) report.keys[idx++] = k;
        else break;
    }

    if (M5Cardputer.Keyboard.isKeyPressed(' ')) {
        const uint8_t HID_SPACE = 0x2C;
        bool present = false;
        for (uint8_t i = 0; i < idx; ++i) if (report.keys[i] == HID_SPACE) { present = true; break; }
        if (!present && idx < 6) report.keys[idx++] = HID_SPACE;
    }

    if (idx == 0 && report.modifiers == 0) {
        keyboard.releaseAll();
    } else {
        keyboard.sendReport(&report);
    }
}
