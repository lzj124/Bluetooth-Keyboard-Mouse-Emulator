#include "display.h"


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
    } else {
        M5Cardputer.Display.setCursor(23, 7);
        M5Cardputer.Display.print("Bluetooth");
    }
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
    M5Cardputer.Display.printf("Version 1.2 - Gyro"); 

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
    M5Cardputer.Display.setTextColor(0x4208);
    M5Cardputer.Display.setTextSize(1);

    if (mouseMode) {
        M5Cardputer.Display.setCursor(8, 116);
        M5Cardputer.Display.print("[Enter]: LMB  [\\]: RMB");
        M5Cardputer.Display.setCursor(8, 125);
        M5Cardputer.Display.print("[;]/[.]: scroll up/down");
    } else {
        if (toggleMode) {
            M5Cardputer.Display.setCursor(8, 120);
            M5Cardputer.Display.print("[Fn]: toggle mouse");
        } else {
            M5Cardputer.Display.setCursor(8, 120);
            M5Cardputer.Display.print("[Fn]: hold= mouse");
        }
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

static const char* settingNames[SETTING_COUNT] = {
    "Sensitivity",
    "Toggle Mode",
    "USB Mode"
};

void displaySettingsScreen(int selected, int sensitivity, bool toggleMode, bool usbMode) {
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    // Title
    M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
    M5Cardputer.Display.setTextSize(1.5);
    M5Cardputer.Display.setCursor(55, 5);
    M5Cardputer.Display.print("Settings");
    M5Cardputer.Display.drawLine(10, 22, 230, 22, 0x4208);

    for (int i = 0; i < SETTING_COUNT; i++) {
        int y = 30 + i * 35;

        // Selection highlight
        if (i == selected) {
            M5Cardputer.Display.drawRoundRect(12, y - 2, 216, 32, 4, TFT_GREEN);
        }

        // Name
        M5Cardputer.Display.setTextColor(i == selected ? TFT_GREEN : 0x8410);
        M5Cardputer.Display.setTextSize(1.2);
        M5Cardputer.Display.setCursor(20, y + 3);
        M5Cardputer.Display.print(settingNames[i]);

        // Value area
        M5Cardputer.Display.setTextSize(1.5);
        M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);

        if (i == SETTING_SENSITIVITY) {
            // Sensitivity: [<] ████████ [>]  N
            M5Cardputer.Display.setCursor(110, y + 1);
            M5Cardputer.Display.print("[<]");
            // Bar
            int barX = 145, barY = y + 6, barW = 60, barH = 10;
            M5Cardputer.Display.drawRect(barX, barY, barW, barH, TFT_LIGHTGRAY);
            int fill = (sensitivity * barW) / 10;
            if (fill > 0) {
                M5Cardputer.Display.fillRect(barX + 1, barY + 1, fill - 1, barH - 2, TFT_GREEN);
            }
            M5Cardputer.Display.print("[>]");
            M5Cardputer.Display.setCursor(210, y + 4);
            M5Cardputer.Display.setTextSize(1);
            M5Cardputer.Display.print(sensitivity);
        } else if (i == SETTING_TOGGLE_MODE) {
            M5Cardputer.Display.setTextSize(1.5);
            M5Cardputer.Display.setTextColor(toggleMode ? TFT_GREEN : 0x8410);
            M5Cardputer.Display.setCursor(110, y + 1);
            M5Cardputer.Display.print("[<] Press [>]");
            M5Cardputer.Display.setTextColor(!toggleMode ? TFT_GREEN : 0x8410);
            M5Cardputer.Display.setCursor(110, y + 18);
            M5Cardputer.Display.print("[<] Hold  [>]");
        } else if (i == SETTING_USB_MODE) {
            M5Cardputer.Display.setTextSize(1.5);
            M5Cardputer.Display.setTextColor(usbMode ? TFT_GREEN : 0x8410);
            M5Cardputer.Display.setCursor(110, y + 1);
            M5Cardputer.Display.print("[<] USB      [>]");
            M5Cardputer.Display.setTextColor(!usbMode ? TFT_GREEN : 0x8410);
            M5Cardputer.Display.setCursor(110, y + 18);
            M5Cardputer.Display.print("[<] Bluetooth[>]");
        }
    }

    // Footer
    M5Cardputer.Display.setTextColor(0x4208);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(15, 128);
    M5Cardputer.Display.print("[;]/[.]: select  [,]/[/]: change  [Go]: back");
}
