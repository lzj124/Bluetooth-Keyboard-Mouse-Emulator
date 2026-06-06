#include "bluetooth.h"
#include "esp_gap_ble_api.h"

BLEHIDDevice* hid;
BLECharacteristic* mouseInput;
BLECharacteristic* keyboardInput;
bool bluetoothIsConnected = false;

void MyBLEServerCallbacks::onConnect(BLEServer* pServer) {
    bluetoothIsConnected = true;
    Serial.println("[BLE] Connected");
}

void MyBLEServerCallbacks::onDisconnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
    bluetoothIsConnected = false;
    Serial.println("[BLE] Disconnected");
    pServer->disconnect(param->disconnect.conn_id);
    pServer->startAdvertising();
}

bool MySecurityCallbacks::onConfirmPIN(uint32_t pin) {
    Serial.printf("[BLE] Confirm PIN: %06d\n", pin);
    return false;
}

uint32_t MySecurityCallbacks::onPassKeyRequest() {
    Serial.println("[BLE] PassKey request — not used");
    return 0;
}

void MySecurityCallbacks::onPassKeyNotify(uint32_t pass_key) {
    Serial.printf("[BLE] PassKey notify: %06d\n", pass_key);
}

bool MySecurityCallbacks::onSecurityRequest() {
    Serial.println("[BLE] Security request — accepting");
    return true;
}

void MySecurityCallbacks::onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
    if (auth_cmpl.success) {
        Serial.println("[BLE] Bond SUCCESS");
    } else {
        Serial.printf("[BLE] Bond FAILED: reason=%d\n", auth_cmpl.fail_reason);
    }
}

bool getBluetoothStatus() {
    return  bluetoothIsConnected;
}

void bluetoothMouse(bool gyroMode) {
    int16_t x = 0;
    int16_t y = 0;
    int8_t wheel = 0;
    uint8_t buttons = 0;

    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    if (status.enter) {
        buttons |= 0x01;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('\\')) {
        buttons |= 0x02;
    }

    // Scroll wheel: ; = up, . = down (in any mouse mode)
    if (M5Cardputer.Keyboard.isKeyPressed(';')) {
        wheel = 1;
    } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
        wheel = -1;
    }

    if (gyroMode) {
        const float DEADZONE = 5.0f;
        const float SENSITIVITY = 0.15f;

        float rawX = gyroZ * cos(tiltAngle) + gyroY * sin(tiltAngle);
        float rawY = gyroX;
        if (abs(rawX) > DEADZONE || abs(rawY) > DEADZONE) {
            x = (int16_t)(-rawX * SENSITIVITY);
            y = (int16_t)(-rawY * SENSITIVITY);
        }
    } else {
        if (M5Cardputer.Keyboard.isKeyPressed('/')) {
            x += 1;
        }
        else if (M5Cardputer.Keyboard.isKeyPressed(',')) {
            x -= 1;
        }
    }

    uint8_t report[5] = {buttons, (uint8_t)x, (uint8_t)y, (uint8_t)wheel, 0};
    mouseInput->setValue(report, sizeof(report));
    mouseInput->notify();
}

void bluetoothKeyboard() {
    uint8_t modifier = 0;
    uint8_t keycode[6] = {0};

    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    int count = 0;
    for (auto key : status.hid_keys) {
        if (count < 6) {
            keycode[count] = key;
            count++;
        }
    }

    if (M5Cardputer.Keyboard.isKeyPressed(' ') && count < 6) {
        keycode[count++] = 0x2C;
    }

    if (status.ctrl)  modifier |= 0x01;
    if (status.shift) modifier |= 0x02;
    if (status.alt)   modifier |= 0x04;

    uint8_t report[8] = {
        modifier, 0,
        keycode[0], keycode[1], keycode[2],
        keycode[3], keycode[4], keycode[5]
    };
    keyboardInput->setValue(report, sizeof(report));
    keyboardInput->notify();

    delay(50);
}

void sendEmptyReports() {
    uint8_t emptyMouseReport[5] = {0, 0, 0, 0, 0};
    mouseInput->setValue(emptyMouseReport, sizeof(emptyMouseReport));
    mouseInput->notify();

    uint8_t emptyKeyboardReport[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    keyboardInput->setValue(emptyKeyboardReport, sizeof(emptyKeyboardReport));
    keyboardInput->notify();
}

void handleBluetoothMode(bool mouseMode, bool gyroMode) {
    if (bluetoothIsConnected) {
        if (M5Cardputer.Keyboard.isPressed()) {
            if (mouseMode) {
                bluetoothMouse(gyroMode);
            } else {
                bluetoothKeyboard();
            }
        } else {
            sendEmptyReports();
        }
    }
    delay(7);
}

void initBluetooth() {
    BLEDevice::init("M5-Keyboard-Mouse");

    // Static BLE address from device MAC — host recognizes us across reboots
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT);
    mac[0] |= 0xC0;
    esp_ble_gap_set_rand_addr(mac);

    Serial.print("[BLE] addr: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X%s", mac[i], i < 5 ? ":" : "\n");
    }

    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyBLEServerCallbacks());

    hid = new BLEHIDDevice(pServer);
    mouseInput = hid->inputReport(1);
    keyboardInput = hid->inputReport(2);

    hid->manufacturer()->setValue("M5Stack");
    hid->pnp(0x02, 0x1234, 0x5678, 0x0100);
    hid->hidInfo(0x00, 0x01);
    hid->reportMap((uint8_t*)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));
    hid->startServices();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(HID_MOUSE);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    static MySecurityCallbacks secCb;
    BLEDevice::setSecurityCallbacks(&secCb);
}

void deinitBluetooth() {
    BLEDevice::deinit();
    delay(1000);
}
