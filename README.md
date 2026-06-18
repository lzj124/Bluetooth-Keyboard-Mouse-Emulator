# M5 Keyboard & Mouse Emulator

M5Cardputer 变身 USB/BLE 双模键鼠，支持陀螺仪鼠标、滚轮、设置页面。即插即用，无需驱动。

## 功能一览

### ⌨️🖱️ 键盘 + 鼠标双模

- **键盘模式** — 全键 QWERTY 键盘通过 HID 输出，支持所有标准按键
- **鼠标模式** — 陀螺仪控制光标，`Enter` 左键、`\` 右键、`;` / `.` 滚轮
- **两种切换方式**（设置中可选）：
  - **Toggle**：按 `Fn` 切换键盘/鼠标模式（再按切回）
  - **Hold**：按住 `Fn` 进入鼠标模式，松开回到键盘

### ⚙️ 设置页面

按 **Go**（侧面按键）进入设置，所有选项保存到 NVS，断电不丢：

| 选项 | 范围 | 说明 |
|------|------|------|
| Sensitivity | 1–10 | 陀螺仪灵敏度 + 方向键速度 |
| Switch | Toggle / Hold | Fn 键切换模式 |
| Mode | USB / Bluetooth | 连接方式，切换即时生效无需重启 |

导航：`;` `.` 上下选，`,` `/` 调节数值，`Go` 或 `Esc` 退出。

### 🎮 陀螺仪鼠标

- 倾斜/旋转 Cardputer 控制光标移动
- 在鼠标模式下自动启用（键盘模式下不触发，省电）
- 硬件：内置 BMI270 IMU，6 轴传感器

### 📜 滚轮

`;` 向上滚动，`.` 向下滚动，带 50ms 防抖冷却，鼠标和键盘模式下均可用。

### 🔋 电池指示

右上角实时显示电量百分比 + 进度条：
- 🟢 绿色：电量 > 20%
- 🔴 红色：电量 ≤ 20%（提醒充电）

BLE 模式下同步上报电池等级给主机。

### 😴 自动休眠

30 秒无操作自动熄灭屏幕，按任意键或晃动设备唤醒。**鼠标模式下不休眠**，避免使用时突然黑屏。

### 📡 USB / BLE 双模

- **USB 模式**：Type-C 直连，免驱识别为 HID 键盘+鼠标
- **BLE 模式**：蓝牙 5.0 BLE，支持 Windows / macOS / Linux / iOS / Android
  - 静态 MAC 地址，配对一次永久有效
  - Battery Service，主机可读取电量
  - 自动重连，断线自动广播

### 🖥️ 主界面

```
┌──────────────────────────────┐
│  [== USB ==]      [████] 85% │  ← 连接状态 + 电池
│                              │
│   ┌──────┐    ┌──────┐       │
│   │ ⌨️    │    │  🖱️  │       │  ← 键盘/鼠标图标
│   └──────┘    └──────┘       │
│                              │
│ [Fn]: hold=mouse  [Go]:Settings │  ← 动态帮助栏
└──────────────────────────────┘
```

USB/BLE 连接状态实时显示，绿色已连接、红色断线。帮助栏根据当前模式动态变化。

## 安装

### 方式一：M5Launcher（推荐）

1. 下载 `release/M5-keyboard-mouse.bin`
2. 放入 SD 卡 `downloads/` 目录
3. M5Launcher 中刷入

### 方式二：M5Burner

在 M5Burner 中搜索 M5CARDPUTER 分类，找到 `M5-Keyboard-Mouse` 烧录。

### 方式三：从源码编译

```bash
pio run
# 固件位于 .pio/build/m5stack-stamps3/firmware.bin
```

编译后可用 M5Launcher WebUI（`http://<设备IP>`）上传刷入，或 `pio run -t upload` 串口烧录。

## 使用说明

### 首次连接

1. 开机显示欢迎画面 → 自动进入主界面
2. 按 **Go** 打开设置，选择 USB 或 Bluetooth 模式
3. **USB**：插线即用
4. **BLE**：手机/电脑搜索 `M5-Keyboard-Mouse` 配对（PIN 码显示在屏幕上）

### 快捷操作

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

### 配对多个设备

BLE 使用固定 MAC 地址，每台设备只需配对一次。切换设备时在新设备上搜索连接即可（旧设备需断开）。

## 技术参数

| 项目 | 详情 |
|------|------|
| 主控 | ESP32-S3 (M5StampS3) |
| 屏幕 | ST7789V2, 240×135, 1.14" IPS |
| 传感器 | BMI270 六轴 IMU（陀螺仪+加速度计） |
| 蓝牙 | BLE 5.0，HID over GATT |
| USB | USB-C OTG，HID 复合设备 |
| 电池 | 120mAh 内置 + 1400mAh 底座 |
| 框架 | Arduino (PlatformIO) |
| 闪存 | 8MB Flash |

## 项目结构

```
src/
├── main.cpp          # 主循环、状态管理、设置页
├── bluetooth.h/cpp   # BLE HID 实现、HID Report Map
├── usbHid.h/cpp      # USB HID 实现
├── display.h/cpp     # UI 渲染（主屏、设置、电池、帮助栏）
└── USBHIDMouse.h     # USB HID 鼠标驱动
```

## 致谢

基于 [geo-tp/Bluetooth-Keyboard-Mouse-Emulator](https://github.com/geo-tp/Bluetooth-Keyboard-Mouse-Emulator) 二次开发。
