#include <M5Cardputer.h>
#include "bluetooth.h"
#include "display.h"
#include "usbHid.h"
#include <USB.h>
#include <Preferences.h>

// ── Persistent settings (stored in NVS "settings") ──
int   mouseSensitivity = 5;    // 1-10
bool  toggleMode = false;      // false=hold-Fn, true=press-Fn-toggle
bool  usbMode = true;

// ── Runtime state ──
bool mouseMode = false;
bool gyroMode = false;
bool gyroAvailable = false;
bool lastBluetoothStatus = false;
unsigned long lastBatteryDraw = 0;
unsigned long lastActivity = 0;
bool screenOn = true;
const unsigned long SLEEP_TIMEOUT = 30000;

// Gyro data
float gyroX = 0.0f, gyroY = 0.0f, gyroZ = 0.0f;
float tiltAngle = 0.0f;

// ── Settings helpers ──
void loadSettings() {
    Preferences p;
    p.begin("settings", true);
    mouseSensitivity = p.getInt("sens", 5);
    toggleMode       = p.getBool("tgl", false);
    usbMode          = p.getBool("usb", true);
    p.end();
    if (mouseSensitivity < 1)  mouseSensitivity = 1;
    if (mouseSensitivity > 10) mouseSensitivity = 10;
}

void saveSettings() {
    Preferences p;
    p.begin("settings", false);
    p.putInt("sens", mouseSensitivity);
    p.putBool("tgl", toggleMode);
    p.putBool("usb", usbMode);
    p.end();
}

// ── Settings page ──
void settingsPage() {
    int sel = 0;
    bool dirty = false;
    displaySettingsScreen(sel, mouseSensitivity, toggleMode, usbMode);

    while (true) {
        M5Cardputer.update();

        // Go button exits (check before keyboard block)
        if (M5Cardputer.BtnA.wasPressed()) {
            break;
        }

        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState ks = M5Cardputer.Keyboard.keysState();

            // Esc also exits
            if (M5Cardputer.Keyboard.isKeyPressed(0x1B)) {
                break;
            }

            // Navigate: ; up, . down
            if (M5Cardputer.Keyboard.isKeyPressed(';')) {
                sel = (sel - 1 + SETTING_COUNT) % SETTING_COUNT;
                dirty = true;
            }
            if (M5Cardputer.Keyboard.isKeyPressed('.')) {
                sel = (sel + 1) % SETTING_COUNT;
                dirty = true;
            }

            // Adjust: , left, / right
            bool left  = M5Cardputer.Keyboard.isKeyPressed(',');
            bool right = M5Cardputer.Keyboard.isKeyPressed('/');
            if (left || right) {
                if (sel == SETTING_SENSITIVITY) {
                    if (left)  mouseSensitivity = max(1,  mouseSensitivity - 1);
                    if (right) mouseSensitivity = min(10, mouseSensitivity + 1);
                } else if (sel == SETTING_TOGGLE_MODE) {
                    if (left || right) toggleMode = !toggleMode;
                } else if (sel == SETTING_USB_MODE) {
                    if (left || right) usbMode = !usbMode;
                }
                dirty = true;
            }
        }

        if (dirty) {
            displaySettingsScreen(sel, mouseSensitivity, toggleMode, usbMode);
            dirty = false;
        }
        delay(10);
    }

    saveSettings();
}

// ── Init ──
void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    loadSettings();

    if (M5.Imu.isEnabled()) {
        gyroAvailable = true;
        float dummy;
        for (int i = 0; i < 10; i++) {
            M5.Imu.update();
            M5.Imu.getGyroData(&dummy, &dummy, &dummy);
            delay(5);
        }
    }

    setupDisplay();
    displayWelcomeScreen();

    // Init selected mode
    if (usbMode) {
        USB.begin();
    } else {
        initBluetooth();
    }

    displayMainScreen(usbMode, mouseMode, getBluetoothStatus(), gyroAvailable && gyroMode);
}

// ── Main loop ──
void loop() {
    M5Cardputer.update();

    // IMU
    if (gyroAvailable) {
        float ax, ay, az;
        M5.Imu.update();
        M5.Imu.getGyroData(&gyroX, &gyroY, &gyroZ);
        M5.Imu.getAccel(&ax, &ay, &az);
        tiltAngle = atan2(ay, az);
    }

    // Activity / sleep
    bool anyKey = M5Cardputer.Keyboard.isPressed();
    bool gyroActive = gyroAvailable && (abs(gyroX) > 10 || abs(gyroY) > 10 || abs(gyroZ) > 10);
    if (anyKey || gyroActive || M5Cardputer.BtnA.isPressed()) {
        lastActivity = millis();
        if (!screenOn) {
            screenOn = true;
            M5Cardputer.Display.setBrightness(128);
        }
    }
    if (screenOn && millis() - lastActivity > SLEEP_TIMEOUT && mouseMode == false) {
        screenOn = false;
        M5Cardputer.Display.setBrightness(0);
    }
    if (!screenOn) {
        delay(50);
        return;
    }

    // ── Go button → Settings ──
    if (M5Cardputer.BtnA.wasPressed()) {
        bool oldUsbMode = usbMode;

        settingsPage();

        // Handle mode switch if changed
        if (usbMode != oldUsbMode) {
            if (usbMode) {
                deinitBluetooth();
                USB.begin();
            } else {
                initBluetooth();
            }
        }
        displayMainScreen(usbMode, mouseMode, getBluetoothStatus(), gyroAvailable && gyroMode);
        lastBluetoothStatus = getBluetoothStatus();
        delay(50);
        return;
    }

    // BT connection status
    auto bluetoothStatus = getBluetoothStatus();
    if (lastBluetoothStatus != bluetoothStatus) {
        modeIndicator(usbMode, bluetoothStatus);
        lastBluetoothStatus = bluetoothStatus;
    }

    // ── Mouse/KB toggle ──
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    bool fnHeld = status.fn;

    if (toggleMode) {
        // Press-toggle: each Fn press flips mouseMode
        static bool fnWasDown = false;
        if (fnHeld && !fnWasDown) {
            mouseMode = !mouseMode;
            drawDeviceRect(mouseMode);
            drawHelpText(mouseMode, toggleMode);
        }
        fnWasDown = fnHeld;
    } else {
        // Hold: Fn held = mouse, release = keyboard
        if (fnHeld && !mouseMode) {
            mouseMode = true;
            drawDeviceRect(mouseMode);
            drawHelpText(mouseMode, toggleMode);
        } else if (!fnHeld && mouseMode) {
            mouseMode = false;
            drawDeviceRect(mouseMode);
            drawHelpText(mouseMode, toggleMode);
        }
    }

    gyroMode = gyroAvailable && (fnHeld || (toggleMode && mouseMode));

    // Battery
    if (millis() - lastBatteryDraw > 5000) {
        drawBattery();
        lastBatteryDraw = millis();
        // Sync battery level to BLE
        if (!usbMode && pBattLevel) {
            uint8_t batt = M5.Power.getBatteryLevel();
            pBattLevel->setValue(&batt, 1);
            pBattLevel->notify();
        }
    }

    // HID
    if (usbMode) {
        handleUsbMode(mouseMode, gyroMode);
    } else {
        handleBluetoothMode(mouseMode, gyroMode);
    }
}
