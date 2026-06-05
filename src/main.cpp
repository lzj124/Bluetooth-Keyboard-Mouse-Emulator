#include <M5Cardputer.h>
#include "bluetooth.h"
#include "display.h"
#include "usbHid.h"
#include <USB.h>

bool mouseMode = true;
bool usbMode = true;
bool gyroMode = false;
bool gyroAvailable = false;
bool lastBluetoothStatus = false;

// Gyro data shared across modules
float gyroX = 0.0f;   // pitch: tilt forward/back → mouse Y
float gyroY = 0.0f;   // roll: wrist left/right → mouse X (when device vertical)
float gyroZ = 0.0f;   // yaw: turn left/right → mouse X (when device flat)
float tiltAngle = 0.0f;  // device tilt from accelerometer (rad)

void selectMode() {
    bool lastMode = !usbMode;
    while (true) {
        M5Cardputer.update();

        if (lastMode != usbMode) {
            displaySelectionScreen(usbMode);
            lastMode = usbMode;
        }

        if (M5Cardputer.Keyboard.isChange()) {
            if (M5Cardputer.Keyboard.isPressed()) {
                Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

                if(M5Cardputer.Keyboard.isKeyPressed('.') || M5Cardputer.Keyboard.isKeyPressed(';')) {
                    usbMode = !usbMode;
                }

                if (status.enter) {
                    break;
                }
            }
        }
        delay(10);
    }
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    
    // M5Cardputer.begin() already inits the IMU — just warm it up if available
    if (M5.Imu.isEnabled()) {
        gyroAvailable = true;
        float dummy;
        for (int i = 0; i < 10; i++) {
            M5.Imu.update();
            M5.Imu.getGyroData(&dummy, &dummy, &dummy);
            delay(5);
        }
        gyroAvailable = true;
    }
    
    setupDisplay();
    displayWelcomeScreen();

    selectMode();
    if (usbMode) {
        USB.begin();
    } else {
        initBluetooth();
    }

    displayMainScreen(usbMode, mouseMode, getBluetoothStatus(), gyroAvailable && gyroMode);
}

void loop() {
    M5Cardputer.update();

    // Read IMU data if available
    if (gyroAvailable) {
        float ax, ay, az;
        M5.Imu.update();
        M5.Imu.getGyroData(&gyroX, &gyroY, &gyroZ);
        M5.Imu.getAccel(&ax, &ay, &az);
        // Tilt: how much device is pointing upward (0=flat, ~1.57=vertical)
        tiltAngle = atan2(ay, az);
    }

    // For BT connection status change
    auto bluetoothStatus = getBluetoothStatus();
    if (lastBluetoothStatus != bluetoothStatus) {
        modeIndicator(usbMode, bluetoothStatus);
        lastBluetoothStatus = bluetoothStatus;
    }

    // Fn key toggles gyro mouse mode (only if IMU available)
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if (gyroAvailable && status.fn) {
        if (!gyroMode) {
            gyroMode = true;
            drawGyroIndicator(true);
        }
    } else {
        if (gyroMode) {
            gyroMode = false;
            drawGyroIndicator(false);
        }
    }

    // Switch between keyboard/mouse
    if (M5Cardputer.BtnA.isPressed()) {
        mouseMode = !mouseMode;
        drawDeviceRect(mouseMode);
        delay(200);
    }

    if (usbMode) {
        handleUsbMode(mouseMode, gyroMode);
    } else {
        handleBluetoothMode(mouseMode, gyroMode);
    }
}
