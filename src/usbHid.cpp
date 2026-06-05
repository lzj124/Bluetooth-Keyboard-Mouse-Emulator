#include "usbHid.h"

USBHIDMouse mouse;
USBHIDKeyboard keyboard;

void handleUsbMode(bool mouseMode, bool gyroMode) {
    if (mouseMode) {
        usbMouse(gyroMode);
    } else  {
        usbKeyboard();
    }
    delay(5);
}

void usbMouse(bool gyroMode) {
    mouse.begin();
    int moveX = 0;
    int moveY = 0;

    if (gyroMode) {
        // Gyroscope control: tilt device to move cursor
        // Deadzone: ignore small movements
        const float DEADZONE = 5.0f;
        const float SENSITIVITY = 0.15f;
        const float ACCEL = 0.003f;  // quadratic boost for fast tilts

        if (abs(gyroZ) > DEADZONE || abs(gyroY) > DEADZONE) {
            // Blend yaw and roll based on device tilt
            float rawX = gyroZ * cos(tiltAngle) + gyroY * sin(tiltAngle);
            if (abs(rawX) > DEADZONE) {
                moveX = (int)(-rawX * (SENSITIVITY + abs(rawX) * ACCEL));
            }
        }
        if (abs(gyroX) > DEADZONE) {
            moveY = (int)(-gyroX * (SENSITIVITY + abs(gyroX) * ACCEL));
        }

        // Button clicks still work in gyro mode
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
        if (status.enter) {
            mouse.press(MOUSE_BUTTON_LEFT);
        } else if (M5Cardputer.Keyboard.isKeyPressed('\\')) {
            mouse.press(MOUSE_BUTTON_RIGHT);
        } else {
            mouse.release(MOUSE_BUTTON_LEFT);
            mouse.release(MOUSE_BUTTON_RIGHT);
        }

        mouse.move(moveX, moveY);
        return;
    }

    // Original arrow-key mouse control
    if (M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        if (M5Cardputer.Keyboard.isKeyPressed('/')) {
            // droite
            moveX = 1;
        } 
        
        if (M5Cardputer.Keyboard.isKeyPressed(',')) {
            // gauche
            moveX = -1;
        }  
        
        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            // haut
            moveY = -1;
        } 
        
        if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            // bas
            moveY = 1;
        }

        // clics souris
        if (status.enter) {
            // gauche
            mouse.press(MOUSE_BUTTON_LEFT);
        } else if (M5Cardputer.Keyboard.isKeyPressed('\\')) {
            // droit
            mouse.press(MOUSE_BUTTON_RIGHT);
        }
        // Send
        mouse.move(moveX, moveY);

    } else {
        mouse.release(MOUSE_BUTTON_LEFT);
        mouse.release(MOUSE_BUTTON_RIGHT);
    }
}

void usbKeyboard() {
    static bool inited = false;
    if (!inited) { keyboard.begin(); inited = true; }

    if (!M5Cardputer.Keyboard.isChange()) return;

    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    KeyReport report = {0};
    report.modifiers = status.modifiers;

    uint8_t idx = 0;
    for (auto k : status.hid_keys) {
        if (idx < 6) report.keys[idx++] = k;
        else break;
    }

    if (M5Cardputer.Keyboard.isKeyPressed(' ')) {
        const uint8_t HID_SPACE = 0x2C;
        bool present = false;
        for (uint8_t i = 0; i < idx; ++i) if (report.keys[i] == HID_SPACE) { present = true; break; }
        if (!present && idx < 6) report.keys[idx++] = HID_SPACE;
    }

    if (idx == 0 && report.modifiers == 0) {
        keyboard.releaseAll();
    } else {
        keyboard.sendReport(&report);
    }
}
