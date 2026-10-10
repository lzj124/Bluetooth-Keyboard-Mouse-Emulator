#ifndef BTHOSTS_H
#define BTHOSTS_H

#include <stdint.h>
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
//  BLE 主机槽位管理（P0/P1）
//   · 槽位信息持久化在 NVS namespace "bthost"
//   · 连接表只存在运行期（掉电即清）
//  注意：Bluedroid 的 esp_ble_bond_dev_t 不保存 address type，
//        Arduino 的 conn_status_t 也不保存对端地址，
//        所以这两样都必须我们自己抓、自己存。
// ─────────────────────────────────────────────────────────────

#define BTHOST_SLOTS    3
#define BTHOST_NAME_LEN 16

struct BtHostSlot {
    bool    used;
    uint8_t addr[6];                    // identity address（bond 列表里的地址）
    uint8_t addrType;                   // 0 = public, 1 = random (BLE_ADDR_TYPE_*)
    char    name[BTHOST_NAME_LEN];
};

// 最近一次对端事件（屏幕诊断用，无串口也能看）
enum BtPeerEvent : uint8_t {
    BT_EV_NONE = 0,
    BT_EV_CONNECT,
    BT_EV_REJECT,
    BT_EV_BOND,
    BT_EV_DISCONNECT,
};

namespace btHosts {

// ── 槽位（持久化）──
void  begin();                                          // 从 NVS 载入
int   activeSlot();                                     // 0..BTHOST_SLOTS-1，未设置返回 0
int   usedCount();
bool  hasAny();
const BtHostSlot& slot(int i);
int   findSlot(const uint8_t addr[6]);                  // 未找到返回 -1
int   claimSlot(const uint8_t addr[6], uint8_t addrType); // 已有→原槽；否则占空槽；满→-1
void  setActiveSlot(int i);
void  setName(int i, const char* name);
void  clearSlot(int i);
void  clearAll();

// ── 连接表（运行期）──
void     onConnected(uint16_t connId, const uint8_t addr[6], uint8_t addrType);
void     onDisconnected(uint16_t connId);
bool     isConnected();
uint16_t connId();
const uint8_t* peerAddr();                              // 未连接返回 nullptr
uint8_t  peerAddrType();
bool     peerMatchesActive();                           // 软件层校验：当前连接是不是选中的槽
void     setLinkSecure(bool secure);                    // 链路是否已加密（HID 特征要求加密）
bool     linkSecure();

// ── 工具 ──
void formatAddr(const uint8_t addr[6], char* out, size_t outLen);   // "AA:BB:CC:DD:EE:FF"
void shortName(const uint8_t addr[6], char* out, size_t outLen);    // "Host-A1B2"

// ── 最近一次对端事件（屏幕诊断用，无串口也能看）──
void           setLastEvent(BtPeerEvent ev, const uint8_t addr[6], uint8_t addrType);
BtPeerEvent    lastEvent();
const uint8_t* lastEventAddr();
uint8_t        lastEventAddrType();
const char*    addrKindLabel(uint8_t addrType);   // "pub" / "rnd" / "rpa"

}  // namespace btHosts

#endif  // BTHOSTS_H
