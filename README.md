# M5 Keyboard & Mouse Emulator

M5Cardputer 变身 USB/BLE 双模键鼠：陀螺仪空中鼠标、点击防抖、滚轮、**BLE 多主机（3 槽切换）**、USB 直连，免驱。

> 本 fork 在原项目基础上新增：**陀螺仪点击防抖**、**BLE 多主机槽位管理**、
> **macOS/Windows 兼容修复**。详细改动见 [CHANGELOG.md](CHANGELOG.md)。

## 功能一览

### ⌨️🖱️ 键盘 + 鼠标双模

- **键盘模式** — 全键 QWERTY 键盘通过 HID 输出
- **鼠标模式** — 陀螺仪控制光标，`Enter` 左键、`\` 右键、`;` / `.` 滚轮
- **两种切换方式**（设置中可选）：
  - **Toggle**：按 `Fn` 切换键盘/鼠标模式（再按切回）
  - **Hold**：按住 `Fn` 进入鼠标模式，松开回到键盘

### 🎯 陀螺仪点击防抖（本 fork 新增）

按 `Enter` 点击时，按键动作会带动整机震动，IMU 采到尖峰 → 光标在按下瞬间产生位移 →
主机把「点击」判定成「拖拽」。本 fork 的处理流水线：

```
中值(3) → 限斜率 → EMA 低通 → 迟滞死区 → 亚像素累加 → ±127 限幅
+ 按键按下沿锁定光标 180ms（检测到持续运动则提前放行，保留拖拽手感）
```

效果：点击不再拖拽、静止不漂移、快甩照样跟手、按住拖动 ~65ms 后接上。
参数在 `src/gyroMouse.h` 顶部，可直接调。

### 📶 BLE 多主机（3 槽，本 fork 新增）

同一时刻连一台，但**记住 3 台主机**，像罗技 Easy-Switch 一样切换：

- `Go → Settings → Hosts` 进入主机菜单
- **切换信道**：选中已用槽 → `Enter`（断开当前 → 只放行目标主机 → 重新广播）
- **新配一台**：选中空槽 → `Enter`，该槽成为配对目标，再在主机端连 `M5-Keyboard-Mouse`
- **断开连接**：`Disconnect now`（保留槽位）
- **删除信道**：选中槽 → `d`（同时删除配对信息）
- **全部清除**：`Forget all`

槽位信息存在 NVS（`bthost`），断电不丢。

### ⚙️ 设置页面

按 **Go**（侧面按键）进入设置：

| 选项 | 范围 | 说明 |
|------|------|------|
| Sensitivity | 1–10 | 陀螺仪灵敏度 + 方向键速度 |
| Switch | Toggle / Hold | Fn 键切换模式 |
| Mode | USB / Bluetooth | 连接方式 |
| Hosts | `1/3 Host-XXXX` | 进入 BLE 主机菜单（`Enter`） |

导航：`;` `.` 上下选，`,` `/` 调节数值，`Go` / `Esc` 退出。

### 🎮 陀螺仪鼠标

- 倾斜/旋转 Cardputer 控制光标移动，硬件为内置 BMI270 六轴 IMU
- 鼠标模式下自动启用（键盘模式不触发，省电）

### 📜 滚轮 / 🔋 电池 / 😴 自动休眠

- 滚轮：`;` 上滚、`.` 下滚，50ms 防抖；BLE 模式下同步上报电量
- 电池：右上角百分比 + 进度条（>20% 绿色 / ≤20% 红色）
- 休眠：30 秒无操作熄屏，按键或晃动唤醒（鼠标模式不休眠）

### 📡 USB / BLE 双模

- **USB 模式**：Type-C 直连，免驱识别为 HID 键盘 + 鼠标
- **BLE 模式**：BLE 5.0 HID over GATT，支持 Windows / macOS / Linux / iOS / Android
  - 静态 MAC 地址，配对一次永久有效
  - 库自带 Battery Service，主机可读取电量
  - 断线自动重新广播

## ⚠️ 使用须知（踩过的坑）

### 1. USB 模式：必须先插线、再开机

开机瞬间若 USB 未接主机，ESP32-S3 的 USB PHY 会留在 USB-Serial-JTAG，
TinyUSB 的 HID 接口不会建立 → 电脑看不到键鼠。
**顺序反了就重拨一次电源开关**（切到 BLE 模式不受影响）。

### 2. Windows 换固件后要先删设备

Windows 按 PnP ID 缓存 HID 描述符与 GATT 结构。换固件后请先在
「设置 → 蓝牙和其他设备」里**删除 `M5-Keyboard-Mouse`**，再重新配对，
否则 HID 驱动可能一直不绑定（设备会显示在「其他设备」里且无输入）。

### 3. 串口日志在 USB 上看不到

本固件 `ARDUINO_USB_CDC_ON_BOOT=0`，`Serial` 走 UART0（不是 USB 口）。
排查看屏幕上的诊断行（见下），或用 UART 转接。

## 🖥️ 屏幕说明

### 主界面

```
┌──────────────────────────────┐
│  [== BT 1/3 ==]   [████] 85% │  ← 连接状态(USB / BT n/3) + 电池
│   ┌──────┐    ┌──────┐       │
│   │ ⌨️    │    │  🖱️  │       │  ← 键盘/鼠标图标
│   └──────┘    └──────┘       │
│ [Fn]: hold=mouse  [Go]:Settings │
└──────────────────────────────┘
```

### Hosts 菜单（诊断信息都在这里）

```
== Hosts ==
linked C4:11:22:33:44:77 rpa sec      ← 地址 / 类型(pub|rnd|rpa) / 链路是否加密
last: BOND 5C:F3:1A:2B:3C:4D rnd      ← 最近事件：conn / REJECT / BOND / disc
sub K:Y M:N  tx:1234 err:0            ← 主机订阅状态 + notify 成功/失败计数
 1  Host-5566  ACTIVE
 2  Host-4477  switch
 3  (empty) PAIRING
 Disconnect now
 Forget all
Enter:pair/switch d:del Go:back
```

| 诊断字段 | 含义 |
|---|---|
| `pub` / `rnd` / `rpa` | 对端地址类型：公有 / 静态随机 / 可解析私有（RPA） |
| `sec` / `NOENC` | 链路是否已加密。HID 输入特征要求加密，`NOENC` 时主机收不到任何输入 |
| `sub K/M` | 主机是否已订阅键盘/鼠标输入报告（CCCD），全 `N` = 主机一个字节都收不到 |
| `tx` / `err` | notify 成功 / 失败次数 |

## 安装

### 方式一：M5Launcher（推荐）

1. 下载 `release/M5-keyboard-mouse.bin`
2. 放入 SD 卡
3. M5Launcher 中选该 bin 刷入

### 方式二：M5Burner

在 M5Burner 中搜索 M5CARDPUTER 分类，找到 `M5-Keyboard-Mouse` 烧录。

### 方式三：从源码编译

```bash
pio run                 # 固件：.pio/build/m5stack-stamps3/firmware.bin
pio run -t upload       # 串口烧录
```

## 首次连接

1. 开机 → 主界面
2. 按 **Go** → 设置 → `Mode` 选 USB 或 Bluetooth
3. **USB**：**先插线、再开机**，即插即用
4. **BLE**：在 `Hosts` 里选中空槽按 `Enter`（屏幕显示 `pairing to slot N`），
   然后在主机上连接 `M5-Keyboard-Mouse`（Just Works，无 PIN 码）

## 快捷操作

| 按键 | 键盘模式 | 鼠标模式 |
|------|:--------:|:--------:|
| `Fn` | 切换/保持鼠标 | — |
| `Enter` | 回车 | 左键点击 |
| `\` | 反斜杠 | 右键点击 |
| `;` | 分号 | 滚轮上 |
| `.` | 句号 | 滚轮下 |
| `,` | 逗号 | 方向左 |
| `/` | 斜杠 | 方向右 |
| `Go` | 打开设置 | 打开设置 |

## 技术参数

| 项目 | 详情 |
|------|------|
| 主控 | ESP32-S3 (M5StampS3) |
| 屏幕 | ST7789V2, 240×135, 1.14" IPS |
| 传感器 | BMI270 六轴 IMU（陀螺仪+加速度计） |
| 蓝牙 | BLE 5.0，HID over GATT（Bluedroid） |
| USB | USB-C OTG，HID 复合设备 |
| 框架 | Arduino (PlatformIO)，Arduino-ESP32 2.0.17 / IDF 4.4.7 |
| 闪存 | 8MB Flash |

## 项目结构

```
src/
├── main.cpp          # 主循环、状态管理、设置页 / Hosts 菜单
├── bluetooth.h/cpp   # BLE HID、多主机槽位（切换/断开/删除）、白名单与广播策略
├── btHosts.h/cpp     # 主机槽位（NVS 持久化）+ 连接表 + 屏幕诊断事件
├── gyroMouse.h/cpp   # 陀螺仪滤波与点击防抖
├── usbHid.h/cpp      # USB HID 实现
├── display.h/cpp     # UI 渲染（主屏、设置、Hosts、电池、帮助栏）
└── USBHIDMouse.h     # USB HID 鼠标驱动
```

## 致谢

基于 [geo-tp/Bluetooth-Keyboard-Mouse-Emulator](https://github.com/geo-tp/Bluetooth-Keyboard-Mouse-Emulator) 二次开发。
