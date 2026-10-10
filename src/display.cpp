#include "display.h"
#include "btHosts.h"
#include "bluetooth.h"


void drawDeviceRect(bool reverse) {
    if (reverse) {
        // GREEN WHITE
        M5Cardputer.Display.drawRoundRect(10, 25, M5Cardputer.Display.width() / 2 - 15, M5Cardputer.Display.height() - 50, 3, TFT_WHITE); // Keyboard mode rectangle
        M5Cardputer.Display.drawRoundRect(M5Cardputer.Display.width() / 2 + 5, 25, M5Cardputer.Display.width() / 2 - 15, M5Cardputer.Display.height() - 50,  3, TFT_GREEN);  // Mouse mode rectangle
    } else {
        // WHITE GREEN
        M5Cardputer.Display.drawRoundRect(10, 25, M5Cardputer.Display.width() / 2 - 15, M5Cardputer.Display.height() - 50, 3, TFT_GREEN);
        M5Cardputer.Display.drawRoundRect(M5Cardputer.Display.width() / 2 + 5, 25, M5Cardputer.Display.width() / 2 - 15, M5Cardputer.Display.height() - 50,  3, TFT_WHITE); 
    }
}

void drawMouseIcon(uint8_t x, uint8_t y) {
    uint8_t w = 25;
    uint8_t h = 35;

    M5Cardputer.Display.fillRoundRect(x, y, 25, 35, 5, TFT_WHITE);
    M5Cardputer.Display.drawLine(x+w/2, y, x+w/2, y+h/2, TFT_BLACK);
}

void drawKeyboardIcon(uint8_t x, uint8_t y) {
    M5Cardputer.Display.fillRect(x, y, 40, 20, TFT_WHITE);

    M5Cardputer.Display.fillRect(x + 2, y + 2, 6, 6, TFT_BLACK);
    M5Cardputer.Display.fillRect(x + 10, y + 2, 6, 6, TFT_BLACK);
    M5Cardputer.Display.fillRect(x + 18, y + 2, 6, 6, TFT_BLACK);
    M5Cardputer.Display.fillRect(x + 26, y + 2, 6, 6, TFT_BLACK);
    M5Cardputer.Display.fillRect(x + 34, y + 2, 6, 6, TFT_BLACK);

    M5Cardputer.Display.fillRect(x + 2, y + 10, 6, 6, TFT_BLACK);
    M5Cardputer.Display.fillRect(x + 10, y + 10, 6, 6, TFT_BLACK);
    M5Cardputer.Display.fillRect(x + 18, y + 10, 6, 6, TFT_BLACK);
    M5Cardputer.Display.fillRect(x + 26, y + 10, 6, 6, TFT_BLACK);
    M5Cardputer.Display.fillRect(x + 34, y + 10, 6, 6, TFT_BLACK);

    M5Cardputer.Display.drawLine(x, y, x, y + 20, TFT_WHITE);
    M5Cardputer.Display.drawLine(x + 40, y, x + 40, y + 20, TFT_WHITE);
    M5Cardputer.Display.drawLine(x, y + 20, x + 40, y + 20, TFT_WHITE);
}

void modeIndicator(bool usbMode, bool bluetoothStatus) {
    M5Cardputer.Display.setTextSize(1.6);
    
    if (bluetoothStatus || usbMode) {
        M5Cardputer.Display.drawRoundRect(10, 4, 104, 18, 5, TFT_GREEN);
        M5Cardputer.Display.setTextColor(TFT_GREEN);

    } else {
        M5Cardputer.Display.drawRoundRect(10, 4, 104, 18, 5, TFT_RED);
        M5Cardputer.Display.setTextColor(TFT_RED);
    }

    if (usbMode) {
        M5Cardputer.Display.setCursor(50, 7);
        M5Cardputer.Display.print("USB");
        return;
    }

    // 蓝牙模式：已配过主机就显示槽位号，例如 "BT 2/3"
    char label[20];
    if (btHosts::usedCount() > 0) {
        snprintf(label, sizeof(label), "BT %d/%d", btHosts::activeSlot() + 1, BTHOST_SLOTS);
    } else {
        snprintf(label, sizeof(label), "Bluetooth");
    }
    int w = M5Cardputer.Display.textWidth(label);
    M5Cardputer.Display.setCursor(10 + (104 - w) / 2, 7);
    M5Cardputer.Display.print(label);
}

void setupDisplay() {
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_BLACK);
}

void displayWelcomeScreen() {
    M5Cardputer.Display.drawRect(9, 47, 220, 40, TFT_LIGHTGRAY);
    M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
    M5Cardputer.Display.setCursor(18, 58);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.printf("M5-Keyboard-Mouse");

    M5Cardputer.Display.setCursor(70, 120);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.printf("Version 1.3 - Hosts"); 

    delay(2000);
}

void drawBattery() {
    int level = M5.Power.getBatteryLevel();
    int w = M5Cardputer.Display.width();
    int barW = 20, barH = 7;
    int barX = w - 66, barY = 8;
    int txX = w - 40, txY = 7;

    M5Cardputer.Display.drawRect(barX, barY, barW, barH, 0x4208);
    int fillW = (level * (barW - 2)) / 100;
    if (fillW > 0) {
        uint16_t fillColor = level > 20 ? 0x07E0 : TFT_RED;
        M5Cardputer.Display.fillRect(barX + 1, barY + 1, fillW, barH - 2, fillColor);
    }

    M5Cardputer.Display.fillRect(txX - 2, txY, 38, 10, TFT_BLACK);
    M5Cardputer.Display.setTextColor(level > 20 ? 0x07E0 : TFT_RED);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(txX, txY);
    M5Cardputer.Display.printf("%d%%", level);
}

void drawHelpText(bool mouseMode, bool toggleMode) {
    M5Cardputer.Display.fillRect(0, 114, M5Cardputer.Display.width(), 21, TFT_BLACK);
    M5Cardputer.Display.setTextColor(0x39E7);
    M5Cardputer.Display.setTextSize(1);

    if (mouseMode) {
        M5Cardputer.Display.setCursor(8, 116);
        M5Cardputer.Display.print("[Enter]: LMB  [\\\\]: RMB [Go]:Settings");
        M5Cardputer.Display.setCursor(8, 125);
        M5Cardputer.Display.print("[;]/[.]: scroll up/down");
    } else {
        M5Cardputer.Display.setCursor(8, 116);
        if (toggleMode) {
            M5Cardputer.Display.print("[Fn]: toggle mouse");
        } else {
            M5Cardputer.Display.print("[Fn]: hold= mouse");
        }
        M5Cardputer.Display.setCursor(120, 116);
        M5Cardputer.Display.setTextColor(0x4208);
        M5Cardputer.Display.print(" [Go]:Settings");
    }
}

void displayMainScreen(bool usbMode, bool mouseMode, bool bluetoothStatus, bool gyroMode) {
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    drawDeviceRect(mouseMode);
    drawMouseIcon(165, 50);
    drawKeyboardIcon(42, 57);
    modeIndicator(usbMode, bluetoothStatus);
    drawBattery();
    drawHelpText(mouseMode, false);  // toggleMode not relevant for initial draw
}

// ── Settings page ──────────────────────────────────────────

void displaySettingsScreen(int selected, int sensitivity, bool toggleMode, bool usbMode) {
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    // Title
    M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
    M5Cardputer.Display.setTextSize(1.2);
    M5Cardputer.Display.setCursor(60, 2);
    M5Cardputer.Display.print("== Settings ==");
    M5Cardputer.Display.drawLine(5, 17, 235, 17, 0x4208);

    static const char* names[SETTING_COUNT] = {"Sensitivity", "Switch", "Mode", "Hosts"};
    int ys[SETTING_COUNT] = {20, 44, 68, 92};

    for (int i = 0; i < SETTING_COUNT; i++) {
        int y = ys[i];
        bool sel = (i == selected);

        // Highlight bg
        if (sel) {
            M5Cardputer.Display.fillRoundRect(5, y - 2, 230, 22, 3, 0x1082);
            M5Cardputer.Display.drawRoundRect(5, y - 2, 230, 22, 3, TFT_GREEN);
        }

        // Name
        M5Cardputer.Display.setTextColor(sel ? TFT_GREEN : 0x8410);
        M5Cardputer.Display.setTextSize(1.2);
        M5Cardputer.Display.setCursor(10, y + 2);
        M5Cardputer.Display.print(names[i]);

        // Value
        M5Cardputer.Display.setTextSize(1.2);

        if (i == SETTING_SENSITIVITY) {
            // [<] bar N [>]
            M5Cardputer.Display.setTextColor(sel ? TFT_GREEN : 0x8410);
            M5Cardputer.Display.setCursor(110, y + 2);
            M5Cardputer.Display.print("<");
            int barW = map(sensitivity, 1, 10, 0, 55);
            M5Cardputer.Display.fillRect(126, y + 6, barW, 7, TFT_GREEN);
            M5Cardputer.Display.drawRect(126, y + 6, 55, 7, TFT_LIGHTGRAY);
            M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
            M5Cardputer.Display.setCursor(186, y + 2);
            M5Cardputer.Display.print(">");
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.setCursor(205, y + 2);
            M5Cardputer.Display.print(sensitivity);
            M5Cardputer.Display.print("/10");
        } else if (i == SETTING_HOSTS) {
            // 蓝牙主机槽位：P2 起可进入子菜单
            M5Cardputer.Display.setCursor(110, y + 2);
            if (usbMode) {
                M5Cardputer.Display.setTextColor(0x4208);
                M5Cardputer.Display.print("BLE mode only");
            } else if (btHosts::usedCount() == 0) {
                M5Cardputer.Display.setTextColor(0x4208);
                M5Cardputer.Display.print("no host paired");
            } else {
                int a = btHosts::activeSlot();
                M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
                M5Cardputer.Display.printf("%d/%d %s", a + 1, BTHOST_SLOTS, btHosts::slot(a).name);
            }
            M5Cardputer.Display.setTextColor(sel ? TFT_GREEN : 0x4208);
            M5Cardputer.Display.setCursor(222, y + 2);
            M5Cardputer.Display.print(">");
        } else {
            // Toggle: two options
            bool optA = (i == SETTING_TOGGLE_MODE) ? toggleMode : usbMode;
            const char* labelA = (i == SETTING_TOGGLE_MODE) ? "Toggle" : "USB";
            const char* labelB = (i == SETTING_TOGGLE_MODE) ? "Hold"  : "Bluetooth";

            M5Cardputer.Display.setTextColor(optA ? TFT_GREEN : 0x8410);
            M5Cardputer.Display.setCursor(110, y + 2);
            M5Cardputer.Display.print(labelA);
            M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
            M5Cardputer.Display.print(" / ");
            M5Cardputer.Display.setTextColor(!optA ? TFT_GREEN : 0x8410);
            M5Cardputer.Display.print(labelB);
        }
    }

    // Footer
    M5Cardputer.Display.setTextColor(0x39E7);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(4, 122);
    M5Cardputer.Display.print("Enter:Hosts menu    Go:back");
}

// ── Hosts 子页（切换 / 断开 / 删除）────────────────────────

void displayHostsScreen(int selected) {
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
    M5Cardputer.Display.setTextSize(1.2);
    M5Cardputer.Display.setCursor(78, 2);
    M5Cardputer.Display.print("== Hosts ==");
    M5Cardputer.Display.drawLine(5, 16, 235, 16, 0x4208);

    // 状态行
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(8, 19);
    int act = btHosts::activeSlot();
    if (btHosts::isConnected() && btHosts::peerAddr() != nullptr) {
        char a[18];
        btHosts::formatAddr(btHosts::peerAddr(), a, sizeof(a));
        M5Cardputer.Display.setTextColor(TFT_GREEN);
        M5Cardputer.Display.printf("linked %s %s %s", a, btHosts::addrKindLabel(btHosts::peerAddrType()),
                                   btHosts::linkSecure() ? "sec" : "NOENC");
    } else if (!btHosts::slot(act).used) {
        // 空槽被选为目标 → 正在等新主机配对进来
        M5Cardputer.Display.setTextColor(TFT_GREEN);
        M5Cardputer.Display.printf("pairing to slot %d - connect new host", act + 1);
    } else {
        M5Cardputer.Display.setTextColor(0x4208);
        M5Cardputer.Display.printf("waiting for slot %d host", act + 1);
    }

    // 最近一次对端事件（诊断用：没有串口日志时靠这一行判断）
    M5Cardputer.Display.setCursor(8, 27);
    M5Cardputer.Display.setTextColor(0x8410);
    if (btHosts::lastEvent() == BT_EV_NONE) {
        M5Cardputer.Display.print("last: -");
    } else {
        const char* kind = "-";
        switch (btHosts::lastEvent()) {
            case BT_EV_CONNECT:    kind = "conn";   break;
            case BT_EV_REJECT:     kind = "REJECT"; break;
            case BT_EV_BOND:       kind = "BOND";   break;
            case BT_EV_DISCONNECT: kind = "disc";   break;
            default: break;
        }
        char a[18];
        btHosts::formatAddr(btHosts::lastEventAddr(), a, sizeof(a));
        M5Cardputer.Display.setTextColor(btHosts::lastEvent() == BT_EV_REJECT ? TFT_RED : 0x8410);
        M5Cardputer.Display.printf("last: %s %s %s", kind, a,
                                   btHosts::addrKindLabel(btHosts::lastEventAddrType()));
    }

    // 收包诊断：订阅状态 + notify 成功/失败计数
    // 主机"连上了但没反应"时，看这行：sub 全 N = 主机没订阅（CCCD 没写成功）
    M5Cardputer.Display.setCursor(8, 35);
    M5Cardputer.Display.setTextColor(0x8410);
    M5Cardputer.Display.printf("sub K:%c M:%c  tx:%d err:%d",
                               btSubscribed(true) ? 'Y' : 'N',
                               btSubscribed(false) ? 'Y' : 'N',
                               btNotifyOk(), btNotifyErr());

    const int y0 = 44, dy = 15;
    for (int i = 0; i < HOSTS_ROW_COUNT; i++) {
        int y = y0 + i * dy;
        bool sel = (i == selected);

        if (sel) {
            M5Cardputer.Display.fillRoundRect(4, y - 2, 232, 13, 3, 0x1082);
            M5Cardputer.Display.drawRoundRect(4, y - 2, 232, 13, 3, TFT_GREEN);
        }
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(8, y);

        if (i < BTHOST_SLOTS) {
            const BtHostSlot& s = btHosts::slot(i);
            M5Cardputer.Display.setTextColor(sel ? TFT_GREEN : TFT_LIGHTGRAY);
            if (!s.used) {
                M5Cardputer.Display.printf("%d  (empty)", i + 1);
                if (i == btHosts::activeSlot()) {
                    M5Cardputer.Display.setTextColor(TFT_GREEN);
                    M5Cardputer.Display.print(" PAIRING");
                } else {
                    M5Cardputer.Display.setTextColor(0x4208);
                    M5Cardputer.Display.print(" Enter:pair");
                }
            } else {
                M5Cardputer.Display.printf("%d  %-9s", i + 1, s.name);
                if (i == btHosts::activeSlot()) {
                    M5Cardputer.Display.setTextColor(TFT_GREEN);
                    M5Cardputer.Display.print(" ACTIVE");
                } else {
                    M5Cardputer.Display.setTextColor(0x4208);
                    M5Cardputer.Display.print(" switch");
                }
            }
        } else if (i == HOSTS_ROW_DISCONNECT) {
            M5Cardputer.Display.setTextColor(sel ? TFT_GREEN : TFT_LIGHTGRAY);
            M5Cardputer.Display.print("Disconnect now");
        } else {
            M5Cardputer.Display.setTextColor(sel ? TFT_RED : 0x8410);
            M5Cardputer.Display.print("Forget all");
        }
    }

    M5Cardputer.Display.setTextColor(0x39E7);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(4, 121);
    M5Cardputer.Display.print("Enter:pair/switch d:del Go:back");
}
