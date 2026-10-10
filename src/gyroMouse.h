#ifndef GYROMOUSE_H
#define GYROMOUSE_H

#include <stdint.h>

// ─────────────────────────────────────────────────────────────
//  陀螺仪空鼠：按键冲击抑制 + 抖动滤波
//  调手感只改这一段
// ─────────────────────────────────────────────────────────────

// ① 按键锁定：按键按下瞬间冻结光标，避开机械冲击
//    太小 → 点击仍可能被识别成拖拽；太大 → 按住后拖动跟手变差
#define GM_CLICK_LOCK_MS        180

//    1 = 按住按键期间完全冻结光标（纯点击，无法拖拽）
//    0 = 只在按下瞬间冻结 GM_CLICK_LOCK_MS，之后按住可拖拽（默认，保留拖拽）
#define GM_FREEZE_WHILE_DOWN    0

// ② 拖动早解锁：锁定期间若检测到「持续」运动，说明用户确实想拖拽，
//    立即解除锁定，避免拖拽手感发木。需连续 GM_DRAG_FRAMES 帧超过门限。
#define GM_DRAG_GATE            40.0f   // 判定门限 (deg/s)
#define GM_DRAG_FRAMES          12      // 连续帧数（≈60ms @200Hz）

// ③ EMA 低通滤波系数：越小越平滑但越迟钝（0.2~0.5 合适）
#define GM_EMA_ALPHA            0.35f

// ④ 限斜率：每帧最大变化量(deg/s)，用来削掉按键那种 1~2 帧的尖峰。
//    只限制「变化速度」，不影响持续快甩，所以不会让快动作失灵。
#define GM_SLEW_LIMIT           80.0f

// ⑤ 迟滞死区(deg/s)：起步要超过 ENTER，停下要低于 EXIT
#define GM_DEADZONE_ENTER       3.0f
#define GM_DEADZONE_EXIT        1.2f

namespace gyroMouse {

// 清空所有滤波/累加状态（切换模式时调用）
void reset();

// 触发一次按键锁定（在按键「按下沿」调用）
void lock(uint32_t ms = GM_CLICK_LOCK_MS);

// 当前是否处于锁定窗口内
bool locked();

// 上报按键是否按下（仅在 GM_FREEZE_WHILE_DOWN=1 时影响输出）
void setButtonDown(bool down);

// 输入原始角速度(deg/s)与灵敏度(1~10)，输出本帧像素增量
// 已包含：中值滤波 → 限斜率 → EMA → 迟滞死区 → 亚像素累加 → ±127 限幅
void update(float rawX, float rawY, int sensitivity, int &dx, int &dy);

}  // namespace gyroMouse

#endif  // GYROMOUSE_H
