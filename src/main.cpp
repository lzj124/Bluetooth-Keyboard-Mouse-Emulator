#include <M5Cardputer.h>
#include "bluetooth.h"
#include "display.h"
#include "usbHid.h"
#include <USB.h>

bool mouseMode = true;
bool usbMode = true;
bool gyroMode = false;
bool gyroAvailable = false;
bool pairingMode = false;
bool lastBluetoothStatus = false;
unsigned long pairingStart = 0;
unsigned long lastBlinkToggle = 0;
bool blinkOn = false;

// Go button long-press tracking
unsigned long goPressStart = 0;
bool goWasPressed = false;

// Gyro data shared across modules
float gyroX = 0.0f;
float gyroY = 0.0f;
float gyroZ = 0.0f;
float tiltAngle = 0.0f;

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
        tiltAngle = atan2(ay, az);
    }

    // --- BtnA: short press = toggle key/mouse, long press (2s) = BT pairing ---
    bool goPressed = M5Cardputer.BtnA.isPressed();
    static bool pairingTriggered = false;

    // Long-press detection
    if (goPressed && !goWasPressed) {
        goPressStart = millis();
    }
    if (goPressed && goWasPressed && millis() - goPressStart > 2000 && !pairingTriggered) {
        if (!usbMode && !pairingMode) {
            pairingMode = true;
            deinitBluetooth();
            initBluetooth();
            pairingStart = millis();
            lastBlinkToggle = millis();
            blinkOn = true;
            modeIndicator(usbMode, false, true);
            pairingTriggered = true;
        }
    }
    // Short-press on release (if not a long press)
    if (!goPressed && goWasPressed && !pairingTriggered && !pairingMode) {
        mouseMode = !mouseMode;
        drawDeviceRect(mouseMode);
    }
    if (!goPressed) {
        pairingTriggered = false;
    }
    goWasPressed = goPressed;

    // Exit pairing mode on connection
    if (pairingMode && getBluetoothStatus()) {
        pairingMode = false;
    }

    // Blink effect during pairing
    if (pairingMode && millis() - lastBlinkToggle > 400) {
        blinkOn = !blinkOn;
        lastBlinkToggle = millis();
        if (blinkOn) {
            modeIndicator(usbMode, false, true);   // blue
        } else {
            M5Cardputer.Display.drawRoundRect(10, 39, 104, 20, 5, TFT_BLACK);  // off
        }
    }
    // Timeout pairing after 60s
    if (pairingMode && millis() - pairingStart > 60000) {
        pairingMode = false;
    }

    // For BT connection status change (non-pairing)
    if (!pairingMode) {
        auto bluetoothStatus = getBluetoothStatus();
        if (lastBluetoothStatus != bluetoothStatus) {
            modeIndicator(usbMode, bluetoothStatus);
            lastBluetoothStatus = bluetoothStatus;
        }
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

    if (usbMode) {
        handleUsbMode(mouseMode, gyroMode);
    } else {
        handleBluetoothMode(mouseMode, gyroMode);
    }
}
