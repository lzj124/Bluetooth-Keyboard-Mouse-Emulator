# CHANGELOG

## v1.3 — 2026-10-10

多主机 + 兼容性版本（本 fork 的主要改动集中在这一版）

### 新增

- **BLE 多主机（3 槽）**：`btHosts` 模块，槽位（地址 / 地址类型 / 名称）持久化到 NVS，
  运行期维护连接表；`Hosts` 子菜单支持**切换信道 / 断开连接 / 删除信道 / 全部清除**，
  选中空槽即为「下一台主机的配对目标」
- **陀螺仪点击防抖**：新增 `gyroMouse` 模块
  （中值3 → 限斜率 → EMA → 迟滞死区 → 亚像素累加 → ±127 限幅，+ 按下沿锁定 180ms）
- **屏幕诊断**：Hosts 页显示链路加密状态、主机订阅状态（CCCD）、notify 成功/失败计数、
  最近一次对端事件（`conn` / `REJECT` / `BOND` / `disc`）与对端地址类型

### 修复

- 陀螺仪鼠标：按 `Enter` 时整机震动导致「点击」被主机判成「拖拽」
- BLE：鼠标输入报告 5 字节 → **4 字节**，与 HID 描述符声明一致
- BLE：**删除重复的 Battery Service（0x180F）**——`BLEHIDDevice` 已自带一个，
  重复 UUID 会破坏 Arduino BLE 的服务映射表，导致 Windows 把设备归入「其他设备」且无输入
- BLE：输入报告及其描述符权限放宽，修复 Windows 在加密建立前订阅失败后不再重试、
  从而导致 `notify()` 被订阅标志挡住的问题
- BLE：HID 位移未限幅导致的 int8 溢出（大甩动会反向）
- BLE：配对模式下已配对老主机抢占配对窗口
- BLE：`claimSlot()` 复用槽位时篡改 active 槽，导致切换目标被抢回

### 变更

- 移除未使用的 `fastled/FastLED` 依赖（源码零引用，M5Cardputer 也不依赖）
- 报告下发降频：内容变化即发 + 100ms 心跳（鼠标）/ 200ms 心跳（空闲），
  松键立即补发空报告

### 文档

- README：补充「USB 必须先插线再开机」「Windows 换固件后先删设备」
  「串口日志走 UART0」等踩坑须知，以及多主机操作与诊断说明

### 开发迭代记录

| 阶段 | 内容 |
|---|---|
| P0 / P1 | 槽位模块 + 连接表（抓 BDA/addr_type）+ 配对写槽 + 屏幕显示 |
| P2 | 切换 / 断开 / 删除 + 白名单广播策略 + 空槽作为配对目标 |
| P2b | 抢配对窗口修复（claimSlot 不再改 active、老主机按 bond 数量识别并踢掉） |
| P2c | 输入报告权限放宽（Windows 订阅）+ 订阅/计数诊断 |
| P2d | 鼠标报告长度 5 → 4 字节 + 空闲降频 |
| P2e | 去掉重复 Battery 服务 → Windows 恢复正常 ✅ |

## v1.2 及之前

上游基线（[geo-tp/Bluetooth-Keyboard-Mouse-Emulator](https://github.com/geo-tp/Bluetooth-Keyboard-Mouse-Emulator)）：
USB/BLE 双模 HID、陀螺仪鼠标、滚轮、电量显示、自动息屏、设置页。
