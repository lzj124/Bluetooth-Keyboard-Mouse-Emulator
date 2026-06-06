# M5 Keyboard and Mouse Emulator

M5Cardputer as a USB/BLE HID keyboard + mouse with gyro control.

## Key Features

- **USB & BLE Dual Mode** — choose USB or Bluetooth at startup
- **Fn-Hold Mouse** — hold `Fn` for mouse mode, release for keyboard (no more clunky toggle)
- **Gyro Mouse** — tilt/rotate the Cardputer to move cursor (auto-enabled with Fn if IMU available)
- **Scroll Wheel** — `;` scroll up, `.` scroll down (50ms cooldown, works in USB & BLE)
- **Mouse Clicks** — `Enter` = left click, `\` = right click
- **Battery Indicator** — progress bar + percentage on screen, green (>20%) / red (≤20%)
- **Auto Sleep** — display turns off after 30s inactivity, wakes on any key/gyro/BtnA
- **Dynamic Help Text** — bottom bar shows key bindings based on current mode
- **BLE Persistence** — static MAC address so host remembers pairing across reboots

## Installation

- **M5Launcher** — copy `release/firmware.bin` to SD card and flash
- **M5Burner** — search M5CARDPUTER section and burn
- **Build from source** — `pio run` with PlatformIO

## Usage

### Mode Selection

At startup, press `;` / `.` to toggle USB/Bluetooth, then `Enter` to confirm.

### Controls

| Key | Keyboard Mode | Mouse Mode |
|-----|:---:|:---:|
| `Fn` (hold) | — | Activate mouse + gyro |
| `Enter` | Return | Left click |
| `\` | Backslash | Right click |
| `/` | Slash | Move left |
| `,` | Comma | Move right |
| `;` | Semicolon | Scroll up |
| `.` | Period | Scroll down |

### Screen

```
┌──────────────────────────────────┐
│ USB ███               ████░ 85% │
│┌──────┐          ┌──────┐       │
││  ⌨️   │          │  🖱️  │       │
││      │          │      │       │
│└──────┘          └──────┘       │
│         [Fn]: switch            │
└──────────────────────────────────┘
```

## Build

```bash
pio run
# firmware at .pio/build/m5stack-stamps3/firmware.bin
```

Pre-built firmware: `release/firmware.bin`

## Credits

Based on [geo-tp/Bluetooth-Keyboard-Mouse-Emulator](https://github.com/geo-tp/Bluetooth-Keyboard-Mouse-Emulator)
