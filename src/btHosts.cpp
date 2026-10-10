#include "btHosts.h"

#include <Preferences.h>
#include <string.h>
#include <stdio.h>

namespace {

Preferences s_prefs;
const char* NVS_NS = "bthost";

BtHostSlot s_slots[BTHOST_SLOTS];
int  s_active = 0;

// 运行期连接表（本固件同一时刻只连一台，所以只记一条）
bool     s_conn = false;
uint16_t s_connId = 0;
uint8_t  s_peerAddr[6] = {0};
uint8_t  s_peerAddrType = 0;

// 最近一次对端事件（诊断显示）
BtPeerEvent s_ev = BT_EV_NONE;
uint8_t     s_evAddr[6] = {0};
uint8_t     s_evType = 0;

// 链路是否已加密（HID 输入特征要求加密链路，未加密则主机会收不到任何输入）
bool s_linkSecure = false;

// 每槽的 NVS key 前缀：s0/s1/s2
void keyFor(int i, const char* suffix, char* out, size_t n) {
    snprintf(out, n, "s%d%s", i, suffix);
}

void loadSlot(int i) {
    char k[8];
    BtHostSlot& s = s_slots[i];
    memset(&s, 0, sizeof(s));

    keyFor(i, "u", k, sizeof(k));
    s.used = s_prefs.getBool(k, false);
    if (!s.used) return;

    keyFor(i, "a", k, sizeof(k));
    if (s_prefs.getBytes(k, s.addr, 6) != 6) {
        s.used = false;                 // 地址不完整 → 视为空槽
        return;
    }
    keyFor(i, "t", k, sizeof(k));
    s.addrType = s_prefs.getUChar(k, 0);
    keyFor(i, "n", k, sizeof(k));
    String nm = s_prefs.getString(k, "");
    snprintf(s.name, sizeof(s.name), "%s", nm.length() ? nm.c_str() : "Host");
}

void saveSlot(int i) {
    char k[8];
    BtHostSlot& s = s_slots[i];

    keyFor(i, "u", k, sizeof(k));   s_prefs.putBool(k, s.used);
    if (!s.used) {
        // 空槽也要把地址清掉，避免下次误读
        keyFor(i, "a", k, sizeof(k)); s_prefs.remove(k);
        keyFor(i, "t", k, sizeof(k)); s_prefs.remove(k);
        keyFor(i, "n", k, sizeof(k)); s_prefs.remove(k);
        return;
    }
    keyFor(i, "a", k, sizeof(k));   s_prefs.putBytes(k, s.addr, 6);
    keyFor(i, "t", k, sizeof(k));   s_prefs.putUChar(k, s.addrType);
    keyFor(i, "n", k, sizeof(k));   s_prefs.putString(k, s.name);
}

void saveActive() {
    s_prefs.putInt("act", s_active);
}

// 静态随机地址 vs 公有地址的启发式判断（bond 记录不带类型）
uint8_t guessAddrType(const uint8_t addr[6]) {
    // 地址最高两比特：11 = 静态随机地址；00 = 公有地址
    return ((addr[0] & 0xC0) == 0xC0) ? 1 : 0;
}

}  // namespace

namespace btHosts {

void begin() {
    s_prefs.begin(NVS_NS, false);
    for (int i = 0; i < BTHOST_SLOTS; i++) loadSlot(i);
    s_active = s_prefs.getInt("act", 0);
    if (s_active < 0 || s_active >= BTHOST_SLOTS) s_active = 0;
    if (!s_slots[s_active].used) {
        // 当前槽是空的，就顺延到第一个已用槽，便于显示
        for (int i = 0; i < BTHOST_SLOTS; i++) {
            if (s_slots[i].used) { s_active = i; break; }
        }
    }

    Serial.printf("[HOSTS] loaded: active=%d used=%d\n", s_active + 1, usedCount());
    for (int i = 0; i < BTHOST_SLOTS; i++) {
        if (!s_slots[i].used) continue;
        char a[18];
        formatAddr(s_slots[i].addr, a, sizeof(a));
        Serial.printf("[HOSTS]  slot %d: %s type=%d name=%s\n",
                      i + 1, a, s_slots[i].addrType, s_slots[i].name);
    }
}

int activeSlot() { return s_active; }
int usedCount() {
    int n = 0;
    for (int i = 0; i < BTHOST_SLOTS; i++) if (s_slots[i].used) n++;
    return n;
}
bool hasAny() { return usedCount() > 0; }
const BtHostSlot& slot(int i) { return s_slots[i]; }

int findSlot(const uint8_t addr[6]) {
    for (int i = 0; i < BTHOST_SLOTS; i++) {
        if (s_slots[i].used && memcmp(s_slots[i].addr, addr, 6) == 0) return i;
    }
    return -1;
}

int claimSlot(const uint8_t addr[6], uint8_t addrType) {
    int i = findSlot(addr);
    if (i >= 0) {
        // 老主机重新认证：只更新类型，**绝不改 active 槽**。
        // 否则在"选空槽准备配对"时会把这台老主机切回当前目标，把配对窗口抢掉。
        s_slots[i].addrType = addrType;
        saveSlot(i);
        Serial.printf("[HOSTS] slot %d reused (active stays %d)\n", i + 1, s_active + 1);
        return i;
    }

    // 优先用当前 active 槽（空的话），否则找第一个空槽
    int target = -1;
    if (!s_slots[s_active].used) target = s_active;
    else {
        for (int k = 0; k < BTHOST_SLOTS; k++) {
            if (!s_slots[k].used) { target = k; break; }
        }
    }
    if (target < 0) {
        Serial.println("[HOSTS] no free slot");
        return -1;
    }

    BtHostSlot& s = s_slots[target];
    s.used = true;
    memcpy(s.addr, addr, 6);
    s.addrType = addrType;
    shortName(addr, s.name, sizeof(s.name));
    saveSlot(target);
    setActiveSlot(target);

    char a[18];
    formatAddr(addr, a, sizeof(a));
    Serial.printf("[HOSTS] stored into slot %d: %s type=%d name=%s\n",
                  target + 1, a, addrType, s.name);
    return target;
}

void setActiveSlot(int i) {
    if (i < 0 || i >= BTHOST_SLOTS) return;
    s_active = i;
    saveActive();
}

void setName(int i, const char* name) {
    if (i < 0 || i >= BTHOST_SLOTS || !s_slots[i].used || !name) return;
    snprintf(s_slots[i].name, sizeof(s_slots[i].name), "%s", name);
    saveSlot(i);
}

void clearSlot(int i) {
    if (i < 0 || i >= BTHOST_SLOTS) return;
    s_slots[i].used = false;
    memset(s_slots[i].addr, 0, 6);
    s_slots[i].addrType = 0;
    s_slots[i].name[0] = '\0';
    saveSlot(i);

    if (!s_slots[s_active].used) {
        for (int k = 0; k < BTHOST_SLOTS; k++) {
            if (s_slots[k].used) { setActiveSlot(k); break; }
        }
    }
}

void clearAll() {
    for (int i = 0; i < BTHOST_SLOTS; i++) clearSlot(i);
    s_active = 0;
    saveActive();
}

// ── 连接表 ──

void onConnected(uint16_t cid, const uint8_t addr[6], uint8_t addrType) {
    s_conn = true;
    s_connId = cid;
    memcpy(s_peerAddr, addr, 6);
    s_peerAddrType = addrType;

    char a[18];
    formatAddr(addr, a, sizeof(a));
    Serial.printf("[HOSTS] connected conn=%u addr=%s type=%d\n", cid, a, addrType);
}

void onDisconnected(uint16_t cid) {
    if (s_conn && cid == s_connId) {
        s_conn = false;
        s_connId = 0;
        Serial.printf("[HOSTS] disconnected conn=%u\n", cid);
    }
}

bool isConnected() { return s_conn; }
uint16_t connId()  { return s_connId; }
const uint8_t* peerAddr() { return s_conn ? s_peerAddr : nullptr; }
uint8_t peerAddrType()    { return s_peerAddrType; }

bool peerMatchesActive() {
    if (!s_conn) return false;
    if (!s_slots[s_active].used) return true;   // 还没配对，任何连接都算有效
    return memcmp(s_slots[s_active].addr, s_peerAddr, 6) == 0;
}

void setLinkSecure(bool secure) { s_linkSecure = secure; }
bool linkSecure()               { return s_linkSecure; }

// ── 工具 ──

void formatAddr(const uint8_t addr[6], char* out, size_t outLen) {
    snprintf(out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X",
             addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

void shortName(const uint8_t addr[6], char* out, size_t outLen) {
    snprintf(out, outLen, "Host-%02X%02X", addr[4], addr[5]);
}

// ── 最近一次对端事件 ──

void setLastEvent(BtPeerEvent ev, const uint8_t addr[6], uint8_t addrType) {
    s_ev = ev;
    memcpy(s_evAddr, addr, 6);
    s_evType = addrType;
}

BtPeerEvent    lastEvent()        { return s_ev; }
const uint8_t* lastEventAddr()    { return s_evAddr; }
uint8_t        lastEventAddrType(){ return s_evType; }

const char* addrKindLabel(uint8_t addrType) {
    // esp_ble_addr_type_t: 0 public, 1 static random, 2 RPA(public id), 3 RPA(random id)
    switch (addrType) {
        case 0:  return "pub";
        case 1:  return "rnd";
        default: return "rpa";
    }
}

}  // namespace btHosts
