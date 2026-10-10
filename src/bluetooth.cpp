#include "bluetooth.h"
#include "esp_gap_ble_api.h"
#include "gyroMouse.h"
#include "btHosts.h"
#include "BLE2902.h"
#include <string.h>

BLEHIDDevice* hid;
BLECharacteristic* mouseInput;
BLECharacteristic* keyboardInput;
bool bluetoothIsConnected = false;

// P2：广播对象与切换状态
static BLEServer* s_server = nullptr;
static BLEAdvertising* s_adv = nullptr;
static bool s_pendingAdv = false;    // 等断开事件后再重配广播
static int  s_bondCountAtConnect = 0;  // 连接瞬间的 bond 数量，用于识别"老主机"

// 打开时：只允许「已在槽位里、且不是当前目标」的公有地址主机被踢掉。
// 随机/RPA 地址无法判定身份，一律放行，避免误踢真正的主机。
#define BT_REJECT_INTRUDERS 1

static void applyAdvPolicy();
static void restartAdvertising();

// ── 诊断：notify 成功/失败计数（屏幕显示，判断主机到底有没有在收包）──
static int s_notifyOk  = 0;
static int s_notifyErr = 0;

class ReportDiagCb : public BLECharacteristicCallbacks {
    void onStatus(BLECharacteristic* c, Status s, uint32_t code) override {
        (void)c; (void)code;
        if (s == BLECharacteristicCallbacks::SUCCESS_NOTIFY)      s_notifyOk++;
        else if (s != BLECharacteristicCallbacks::SUCCESS_INDICATE) s_notifyErr++;
    }
};
static ReportDiagCb s_reportDiag;

// 统一成「identity address 的类型」：0 = public，1 = static random。
// 连接事件在 RPA 场景下会给出 2/3，不能直接当身份地址类型用。
static uint8_t identityAddrType(const uint8_t addr[6], uint8_t evtType) {
    if (evtType == 0 || evtType == 1) return evtType;
    return ((addr[0] & 0xC0) == 0xC0) ? 1 : 0;
}

// 配对成功后把主机写进槽位。用 bond 列表里的 identity address，
// 因为对端（手机/macOS）常用会轮换的 RPA，连接瞬间的地址不能长期使用。
static int storeBondedHost(const esp_ble_auth_cmpl_t& auth) {
    static esp_ble_bond_dev_t list[8];   // 回调在 BTC 任务里跑，避免吃栈

    int num = esp_ble_get_bond_device_num();
    if (num <= 0) {
        Serial.println("[BLE] bond list empty, storing auth addr");
        return btHosts::claimSlot(auth.bd_addr, (uint8_t)auth.addr_type);
    }
    if (num > 8) num = 8;
    int cnt = num;
    if (esp_ble_get_bond_device_list(&cnt, list) != ESP_OK) {
        return btHosts::claimSlot(auth.bd_addr, (uint8_t)auth.addr_type);
    }

    int pick = -1;
    for (int i = 0; i < cnt; i++) {
        if (memcmp(list[i].bd_addr, auth.bd_addr, 6) == 0) { pick = i; break; }
    }
    if (pick < 0) {
        // 认证事件里是 RPA、bond 列表里是 identity address → 挑一个还没进槽的
        for (int i = 0; i < cnt; i++) {
            if (btHosts::findSlot(list[i].bd_addr) < 0) { pick = i; break; }
        }
    }
    if (pick < 0) pick = 0;

    uint8_t type = identityAddrType(list[pick].bd_addr, (uint8_t)auth.addr_type);
    if (memcmp(list[pick].bd_addr, auth.bd_addr, 6) != 0) {
        Serial.println("[BLE] note: peer uses resolvable address, storing identity address");
    }

    char a[18];
    btHosts::formatAddr(list[pick].bd_addr, a, sizeof(a));
    Serial.printf("[BLE] bond list %d/%d → store %s (type=%d)\n", pick + 1, cnt, a, type);
    return btHosts::claimSlot(list[pick].bd_addr, type);
}

void MyBLEServerCallbacks::onConnect(BLEServer* pServer) {
    // 实际逻辑在带 param 的重载里（库会同时调用两个）
}

void MyBLEServerCallbacks::onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
    bluetoothIsConnected = true;
    // 对端地址与地址类型必须在这里抓：Arduino 的连接表不存地址，bond 记录不存类型
    btHosts::onConnected(param->connect.conn_id,
                         param->connect.remote_bda,
                         (uint8_t)param->connect.ble_addr_type);
    btHosts::setLastEvent(BT_EV_CONNECT, param->connect.remote_bda,
                          (uint8_t)param->connect.ble_addr_type);
    s_bondCountAtConnect = esp_ble_get_bond_device_num();
    Serial.println("[BLE] Connected");

#if BT_REJECT_INTRUDERS
    // 白名单过滤的软件兜底。只对「公有地址」生效：随机/RPA 地址无法判定身份，一律放行。
    //   ① 当前槽有主机：只让该槽的主机连（其余踢掉）
    //   ② 当前槽为空（"pairing to slot N" 模式）：只让陌生主机连，
    //      已配对过的老主机连上来会被踢掉，避免它抢占配对窗口
    if ((uint8_t)param->connect.ble_addr_type == 0) {
        bool activeEmpty = !btHosts::slot(btHosts::activeSlot()).used;
        int  matched     = btHosts::findSlot(param->connect.remote_bda);
        bool reject      = activeEmpty ? (matched >= 0)
                                       : (btHosts::hasAny() && matched != btHosts::activeSlot());
        if (reject) {
            char a[18];
            btHosts::formatAddr(param->connect.remote_bda, a, sizeof(a));
            Serial.printf("[BLE] reject %s (matched slot %d, active %d, pairing=%d)\n",
                          a, matched, btHosts::activeSlot(), (int)activeEmpty);
            btHosts::setLastEvent(BT_EV_REJECT, param->connect.remote_bda,
                                  (uint8_t)param->connect.ble_addr_type);
            esp_ble_gap_disconnect((uint8_t*)param->connect.remote_bda);
        }
    }
#endif
}

void MyBLEServerCallbacks::onDisconnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
    bluetoothIsConnected = false;
    uint8_t prevType = btHosts::peerAddrType();
    btHosts::onDisconnected(param->disconnect.conn_id);
    btHosts::setLinkSecure(false);
    btHosts::setLastEvent(BT_EV_DISCONNECT, param->disconnect.remote_bda, prevType);
    Serial.println("[BLE] Disconnected");
    pServer->disconnect(param->disconnect.conn_id);

    if (s_pendingAdv) {
        s_pendingAdv = false;
        applyAdvPolicy();          // 切槽时先停广播、断开，这里再按新目标配置并开播
        restartAdvertising();
        Serial.println("[BLE] adv restarted for new target slot");
    } else {
        applyAdvPolicy();
        restartAdvertising();
    }
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
        btHosts::setLinkSecure(true);      // 加密链路已建立（Windows 收不到输入的常见原因就是这一步没成）

        // 「选空槽配对」期间，老主机会用已有 bond 直接加密连上来抢窗口。
        // 判据：认证成功但 bond 数量没增加 → 用的是既有 bond → 老主机，踢掉。
        bool activeEmpty = !btHosts::slot(btHosts::activeSlot()).used;
        int  now = esp_ble_get_bond_device_num();
        if (activeEmpty && btHosts::hasAny() && now <= s_bondCountAtConnect) {
            char a[18];
            btHosts::formatAddr(auth_cmpl.bd_addr, a, sizeof(a));
            Serial.printf("[BLE] old host %s grabbed pairing window (bonds %d→%d) → kick\n",
                          a, s_bondCountAtConnect, now);
            btHosts::setLastEvent(BT_EV_REJECT, auth_cmpl.bd_addr, (uint8_t)auth_cmpl.addr_type);
            esp_ble_gap_disconnect((uint8_t*)auth_cmpl.bd_addr);
            return;
        }

        int s = storeBondedHost(auth_cmpl);
        if (s < 0) {
            Serial.println("[BLE] no free host slot — 需要先在设置页清一个槽");
        } else {
            btHosts::setLastEvent(BT_EV_BOND, auth_cmpl.bd_addr, (uint8_t)auth_cmpl.addr_type);
        }
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

    // 切换陀螺仪/方向键模式时清掉滤波状态与按键沿状态
    static bool wasGyro = false;
    static bool prevLeft = false, prevRight = false;
    if (gyroMode != wasGyro) {
        gyroMouse::reset();
        prevLeft = prevRight = false;
        wasGyro = gyroMode;
    }

    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    bool leftClick  = status.enter;
    bool rightClick = M5Cardputer.Keyboard.isKeyPressed('\\');

    if (leftClick) {
        buttons |= 0x01;
    }
    if (rightClick) {
        buttons |= 0x02;
    }

    // 按下沿 → 锁定光标，避免「点击」被识别成「拖拽」
    if ((leftClick && !prevLeft) || (rightClick && !prevRight)) {
        gyroMouse::lock();
    }
    prevLeft  = leftClick;
    prevRight = rightClick;
    gyroMouse::setButtonDown(leftClick || rightClick);

    // Scroll wheel: ; = up, . = down (in any mouse mode) — 50ms cooldown
    static unsigned long lastWheelTime = 0;
    if (millis() - lastWheelTime > 50) {
        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            wheel = 1;
            lastWheelTime = millis();
        } else if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            wheel = -1;
            lastWheelTime = millis();
        }
    }

    if (gyroMode) {
        float rawX = gyroZ * cos(tiltAngle) + gyroY * sin(tiltAngle);
        float rawY = gyroX;
        int dx = 0, dy = 0;
        gyroMouse::update(rawX, rawY, mouseSensitivity, dx, dy);
        x = (int16_t)dx;
        y = (int16_t)dy;
    } else {
        float spd = (mouseSensitivity + 1) / 2;  // 1 ~ 5
        if (M5Cardputer.Keyboard.isKeyPressed('/')) {
            x += (int16_t)spd;
        }
        else if (M5Cardputer.Keyboard.isKeyPressed(',')) {
            x -= (int16_t)spd;
        }
    }

    // 鼠标报告严格按 HID 描述符的 4 字节：buttons, X, Y, wheel
    // （原实现发 5 字节，多了个尾部 0。macOS 容忍，Windows 会判为报告长度不符）
    uint8_t report[4] = {buttons, (uint8_t)x, (uint8_t)y, (uint8_t)wheel};

    // 只在内容变化或 100ms 心跳时下发，避免空闲时刷爆主机
    static uint8_t       lastReport[4] = {0, 0, 0, 0};
    static unsigned long lastSend = 0;
    if (memcmp(report, lastReport, sizeof(report)) != 0 || millis() - lastSend > 100) {
        lastSend = millis();
        memcpy(lastReport, report, sizeof(report));
        mouseInput->setValue(report, sizeof(report));
        mouseInput->notify();
    }
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
    uint8_t emptyMouseReport[4] = {0, 0, 0, 0};   // 与描述符一致：4 字节
    mouseInput->setValue(emptyMouseReport, sizeof(emptyMouseReport));
    mouseInput->notify();

    uint8_t emptyKeyboardReport[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    keyboardInput->setValue(emptyKeyboardReport, sizeof(emptyKeyboardReport));
    keyboardInput->notify();
}

void handleBluetoothMode(bool mouseMode, bool gyroMode) {
    if (!bluetoothIsConnected) {
        delay(7);
        return;
    }

    static unsigned long lastIdle = 0;
    if (mouseMode) {
        bluetoothMouse(gyroMode);
    } else if (M5Cardputer.Keyboard.isPressed()) {
        bluetoothKeyboard();
    } else if (M5Cardputer.Keyboard.isChange() || millis() - lastIdle > 200) {
        // 松键要立刻发空报告清掉按键状态，其余时间降频心跳即可
        lastIdle = millis();
        sendEmptyReports();
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

    // ★ Windows 兼容：BLEHIDDevice::inputReport() 默认把输入报告及其描述符
    //   （含 CCCD 0x2902、Report Reference 0x2908）设成「必须加密」。
    //   Windows 常在加密建立前就去写 CCCD，拿到 Insufficient Authentication 后不再重试，
    //   于是订阅标志一直为 false → notify() 被挡住 → 主机一个字节都收不到。
    //   这里把输入报告相关权限放宽，其余特征（HID Control 等）仍保持默认。
    auto relaxPerm = [](BLECharacteristic* c) {
        if (c == nullptr) return;
        c->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
        BLEDescriptor* d = c->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
        if (d) d->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
        d = c->getDescriptorByUUID(BLEUUID((uint16_t)0x2908));
        if (d) d->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
    };
    relaxPerm(mouseInput);
    relaxPerm(keyboardInput);

    // 诊断回调：统计 notify 成功/失败，屏幕上一眼看出主机到底有没有在收
    mouseInput->setCallbacks(&s_reportDiag);
    keyboardInput->setCallbacks(&s_reportDiag);

    hid->manufacturer()->setValue("M5Stack");
    hid->pnp(0x02, 0x1234, 0x5678, 0x0100);
    hid->hidInfo(0x00, 0x01);
    hid->reportMap((uint8_t*)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));
    hid->startServices();

    // 注意：不要在这里再建 Battery Service！
    // BLEHIDDevice 构造时已建好 0x180F，并在 startServices() 里启动；
    // 再建一个同 UUID 的服务会让 Arduino BLE 的服务映射表冲突
    // （库源码 BLEServer.cpp 里对这个场景就写着 "very big bug for multi services with the same uuid"），
    // Windows 的服务发现会因此拿不到 HID 服务，设备被丢进「其他设备」且没有输入。
    // 电量上报请用 hid->setBatteryLevel()。

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(0x03C0);  // Generic HID (keyboard+mouse combo)
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());

    s_server = pServer;
    s_adv = pAdvertising;
    applyAdvPolicy();       // 已配对过 → 只允许当前槽的主机连上来
    pAdvertising->start();

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    static MySecurityCallbacks secCb;
    BLEDevice::setSecurityCallbacks(&secCb);
}

// ── 主机槽位操作（P2）────────────────────────────────────────

bool btSubscribed(bool keyboard) {
    BLECharacteristic* c = keyboard ? keyboardInput : mouseInput;
    if (c == nullptr) return false;
    BLE2902* cccd = (BLE2902*)c->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    return cccd != nullptr && cccd->getNotifications();
}

int btNotifyOk()  { return s_notifyOk; }
int btNotifyErr() { return s_notifyErr; }

// ── 主机槽位操作（P2）────────────────────────────────────────

static esp_ble_wl_addr_type_t wlType(uint8_t t) {
    return (t == 1) ? BLE_WL_ADDR_TYPE_RANDOM : BLE_WL_ADDR_TYPE_PUBLIC;
}

// 按当前 active 槽重配白名单与广播过滤策略
static void applyAdvPolicy() {
    if (!s_adv) return;

    esp_ble_gap_clear_whitelist();

    int a = btHosts::activeSlot();
    if (!btHosts::hasAny() || !btHosts::slot(a).used) {
        // 还没配过主机 → 开放广播，任何主机都能来配对
        s_adv->setScanFilter(false, false);
        Serial.println("[BLE] adv policy: open (no host paired)");
        return;
    }

    const BtHostSlot& s = btHosts::slot(a);
    esp_err_t err = esp_ble_gap_update_whitelist(true, (uint8_t*)s.addr, wlType(s.addrType));
    s_adv->setScanFilter(false, true);      // 只允许白名单设备发起连接
    char mb[18];
    btHosts::formatAddr(s.addr, mb, sizeof(mb));
    Serial.printf("[BLE] adv policy: whitelist %s type=%d (rc=%d)\n", mb, s.addrType, err);
}

static void restartAdvertising() {
    if (s_adv) {
        s_adv->start();
    } else if (s_server) {
        s_server->startAdvertising();
    }
}

bool btDisconnectCurrent() {
    if (!bluetoothIsConnected || btHosts::peerAddr() == nullptr) return false;
    char a[18];
    btHosts::formatAddr(btHosts::peerAddr(), a, sizeof(a));
    Serial.printf("[BLE] disconnect current %s\n", a);
    esp_ble_gap_disconnect((uint8_t*)btHosts::peerAddr());
    return true;
}

void btSwitchToSlot(int slot) {
    if (slot < 0 || slot >= BTHOST_SLOTS) return;

    if (!btHosts::slot(slot).used) {
        // 空槽 = 把它设为「下一台主机的配对目标」：
        // active 指向空槽后 claimSlot() 会优先占用它，策略回到开放广播
        btHosts::setActiveSlot(slot);
        Serial.printf("[BLE] slot %d empty → waiting for new host to pair\n", slot + 1);

        if (s_adv) s_adv->stop();
        if (bluetoothIsConnected && btHosts::peerAddr() != nullptr) {
            s_pendingAdv = true;
            esp_ble_gap_disconnect((uint8_t*)btHosts::peerAddr());
        } else {
            applyAdvPolicy();
            restartAdvertising();
        }
        return;
    }

    btHosts::setActiveSlot(slot);
    Serial.printf("[BLE] switch → slot %d\n", slot + 1);

    if (s_adv) s_adv->stop();       // 先停广播，避免旧策略继续把人放进来

    if (bluetoothIsConnected && btHosts::peerAddr() != nullptr) {
        s_pendingAdv = true;        // 等断开事件回调里再配置并开播
        esp_ble_gap_disconnect((uint8_t*)btHosts::peerAddr());
    } else {
        applyAdvPolicy();
        restartAdvertising();
    }
}

void btForgetSlot(int slot) {
    if (slot < 0 || slot >= BTHOST_SLOTS) return;

    if (btHosts::slot(slot).used) {
        const uint8_t* addr = btHosts::slot(slot).addr;
        char a[18];
        btHosts::formatAddr(addr, a, sizeof(a));

        // 如果正连着这台，先断开
        if (bluetoothIsConnected && btHosts::peerAddr() != nullptr
            && memcmp(btHosts::peerAddr(), addr, 6) == 0) {
            esp_ble_gap_disconnect((uint8_t*)btHosts::peerAddr());
        }
        esp_err_t err = esp_ble_remove_bond_device((uint8_t*)addr);
        Serial.printf("[BLE] forget slot %d (%s), remove bond rc=%d\n", slot + 1, a, err);
    }

    btHosts::clearSlot(slot);
    applyAdvPolicy();
    if (!bluetoothIsConnected) restartAdvertising();
}

void btForgetAll() {
    for (int i = 0; i < BTHOST_SLOTS; i++) {
        if (!btHosts::slot(i).used) continue;
        esp_ble_remove_bond_device((uint8_t*)btHosts::slot(i).addr);
    }
    if (bluetoothIsConnected && btHosts::peerAddr() != nullptr) {
        esp_ble_gap_disconnect((uint8_t*)btHosts::peerAddr());
    }
    btHosts::clearAll();
    applyAdvPolicy();
    if (!bluetoothIsConnected) restartAdvertising();
    Serial.println("[BLE] all host slots cleared");
}

void deinitBluetooth() {
    bluetoothIsConnected = false;
    s_adv = nullptr;
    s_server = nullptr;
    s_pendingAdv = false;
    BLEDevice::deinit();
    delay(100);
}
