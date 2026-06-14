# M5 Keyboard and Mouse Emulator

M5Cardputer as a USB/BLE HID keyboard + mouse with gyro control and settings UI.

## Key Features

- **USB & BLE Dual Mode** — switch in settings, no more boot-time selection
- **Settings Page** — press `Go` (side button) to enter: sensitivity, switch mode, USB/BT
- **Toggle / Hold Mouse** — Switch mode: Press Fn to toggle mouse on/off; Hold mode: hold Fn for mouse
- **Gyro Mouse** — tilt/rotate Cardputer to move cursor (auto in mouse mode)
- **Scroll Wheel** — `;` scroll up, `.` scroll down (50ms cooldown)
- **Mouse Clicks** — `Enter` = left click, `\` = right click
- **Battery Indicator** — progress bar + percentage, green (>20%) / red (≤20%)
- **Auto Sleep** — display off after 30s inactivity, disabled in mouse mode
- **Dynamic Help Text** — bottom bar shows bindings + `[Go]:Settings`
- **BLE Persistence** — static MAC address for stable pairing
- **Settings Persistence** — all settings saved to NVS, survive reboot

## Installation

- **M5Launcher** — copy `release/M5-keyboard-mouse.bin` to SD `downloads/` and flash
- **M5Burner** — search M5CARDPUTER section and burn
- **Build from source** — `pio run` with PlatformIO

## Usage

### Settings

Press **Go** (side button) to open settings. Navigate:

| Key | Action |
|-----|--------|
| `;` / `.` | Previous / next option |
| `,` / `/` | Adjust value |
| `Go` / `Esc` | Exit settings |

Settings options:

| Option | Values |
|--------|--------|
| Sensitivity | 1 – 10 (gyro + arrow speed) |
| Switch | Toggle (press Fn to flip) / Hold (hold Fn for mouse) |
| Mode | USB / Bluetooth |

### Controls

| Key | Keyboard Mode | Mouse Mode |
|-----|:---:|:---:|
| `Fn` | Switch/toggle mouse | — |
| `Enter` | Return | Left click |
| `\` | Backslash | Right click |
| `/` | Slash | Move right |
| `,` | Comma | Move left |
| `;` | Semicolon | Scroll up |
| `.` | Period | Scroll down |

## Build

```bash
pio run
# firmware at .pio/build/m5stack-stamps3/firmware.bin
```

Pre-built firmware: `release/M5-keyboard-mouse.bin`

## Credits

Based on [geo-tp/Bluetooth-Keyboard-Mouse-Emulator](https://github.com/geo-tp/Bluetooth-Keyboard-Mouse-Emulator)
