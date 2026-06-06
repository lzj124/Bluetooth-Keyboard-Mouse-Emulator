#include <M5Cardputer.h>
#include "bluetooth.h"
#include "display.h"
#include "usbHid.h"
#include <USB.h>

bool mouseMode = false;  // default keyboard, Fn toggles mouse
bool usbMode = true;
bool gyroMode = false;
bool gyroAvailable = false;
bool lastBluetoothStatus = false;
unsigned long lastBatteryDraw = 0;
unsigned long lastActivity = 0;
bool screenOn = true;
const unsigned long SLEEP_TIMEOUT = 30000;  // 30s

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

    // Activity detection: any key press or gyro movement = awake
    bool anyKey = M5Cardputer.Keyboard.isPressed();
    bool gyroActive = gyroAvailable && (abs(gyroX) > 10 || abs(gyroY) > 10 || abs(gyroZ) > 10);
    if (anyKey || gyroActive || M5Cardputer.BtnA.isPressed()) {
        lastActivity = millis();
        if (!screenOn) {
            screenOn = true;
            M5Cardputer.Display.setBrightness(128);
        }
    }

    // Auto sleep
    if (screenOn && millis() - lastActivity > SLEEP_TIMEOUT) {
        screenOn = false;
        M5Cardputer.Display.setBrightness(0);
    }

    if (!screenOn) {
        delay(50);
        return;
    }

    // For BT connection status change
    auto bluetoothStatus = getBluetoothStatus();
    if (lastBluetoothStatus != bluetoothStatus) {
        modeIndicator(usbMode, bluetoothStatus);
        lastBluetoothStatus = bluetoothStatus;
    }

    // Fn key: hold = mouse mode, release = keyboard mode; also enables gyro if IMU available
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    bool fnHeld = status.fn;
    
    if (fnHeld && !mouseMode) {
        mouseMode = true;
        drawDeviceRect(mouseMode);
        drawHelpText(mouseMode);
    } else if (!fnHeld && mouseMode) {
        mouseMode = false;
        drawDeviceRect(mouseMode);
        drawHelpText(mouseMode);
    }
    
    gyroMode = gyroAvailable && fnHeld;

    // Periodic battery update
    if (millis() - lastBatteryDraw > 5000) {
        drawBattery();
        lastBatteryDraw = millis();
    }

    if (usbMode) {
        handleUsbMode(mouseMode, gyroMode);
    } else {
        handleBluetoothMode(mouseMode, gyroMode);
    }
}
