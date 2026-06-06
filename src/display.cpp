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

    // Corps de la souris
    M5Cardputer.Display.fillRoundRect(x, y, 25, 35, 5, TFT_WHITE);

    // Ligne boutons
    M5Cardputer.Display.drawLine(x+w/2, y, x+w/2, y+h/2, TFT_BLACK);
}

void drawKeyboardIcon(uint8_t x, uint8_t y) {
    // Dessiner le contour du clavier
    M5Cardputer.Display.fillRect(x, y, 40, 20, TFT_WHITE);

    // Dessiner les touches du clavier
    M5Cardputer.Display.fillRect(x + 2, y + 2, 6, 6, TFT_BLACK);   // Touche 1
    M5Cardputer.Display.fillRect(x + 10, y + 2, 6, 6, TFT_BLACK);  // Touche 2
    M5Cardputer.Display.fillRect(x + 18, y + 2, 6, 6, TFT_BLACK);  // Touche 3
    M5Cardputer.Display.fillRect(x + 26, y + 2, 6, 6, TFT_BLACK);  // Touche 4
    M5Cardputer.Display.fillRect(x + 34, y + 2, 6, 6, TFT_BLACK);  // Touche 5

    M5Cardputer.Display.fillRect(x + 2, y + 10, 6, 6, TFT_BLACK);  // Touche 6
    M5Cardputer.Display.fillRect(x + 10, y + 10, 6, 6, TFT_BLACK); // Touche 7
    M5Cardputer.Display.fillRect(x + 18, y + 10, 6, 6, TFT_BLACK); // Touche 8
    M5Cardputer.Display.fillRect(x + 26, y + 10, 6, 6, TFT_BLACK); // Touche 9
    M5Cardputer.Display.fillRect(x + 34, y + 10, 6, 6, TFT_BLACK); // Touche 10

    // Dessiner les lignes pour les contours
    M5Cardputer.Display.drawLine(x, y, x, y + 20, TFT_WHITE);         // Ligne gauche
    M5Cardputer.Display.drawLine(x + 40, y, x + 40, y + 20, TFT_WHITE); // Ligne droite
    M5Cardputer.Display.drawLine(x, y, x + 40, y, TFT_WHITE);         // Ligne haut
    M5Cardputer.Display.drawLine(x, y + 20, x + 40, y + 20, TFT_WHITE);  // Ligne bas
}

void modeIndicator(bool usbMode, bool bluetoothStatus) {
    M5Cardputer.Display.setTextSize(1.6);
    
    if (bluetoothStatus || usbMode) {
        // Green
        M5Cardputer.Display.drawRoundRect(10, 4, 104, 18, 5, TFT_GREEN);
        M5Cardputer.Display.setTextColor(TFT_GREEN);

    } else {
        // Red
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

void displaySelectionScreen(bool mode) {
    M5Cardputer.Display.clear();
    M5Cardputer.Display.setTextSize(1.5);
    M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
    M5Cardputer.Display.setCursor(70, 10);
    M5Cardputer.Display.printf("Select Mode:");
    M5Cardputer.Display.setTextSize(3);

    // USB SELECTED
    if (mode) {
        M5Cardputer.Display.fillRect(20, 30, 200, 40, TFT_LIGHTGRAY);
        M5Cardputer.Display.drawRect(20, 30, 200, 40, TFT_BLACK);
        M5Cardputer.Display.setTextColor(TFT_BLACK);
    } else {
        M5Cardputer.Display.fillRect(20, 30, 200, 40, TFT_BLACK);
        M5Cardputer.Display.drawRect(20, 30, 200, 40, TFT_LIGHTGRAY);
        M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
    }
    M5Cardputer.Display.setCursor(95, 40);
    M5Cardputer.Display.printf("USB");

    // BT SELECTED
    if (!mode) {
        M5Cardputer.Display.fillRect(20, 80, 200, 40, TFT_LIGHTGRAY);
        M5Cardputer.Display.drawRect(20, 80, 200, 40, TFT_BLACK);
        M5Cardputer.Display.setTextColor(TFT_BLACK);
    } else {
        M5Cardputer.Display.fillRect(20, 80, 200, 40, TFT_BLACK);
        M5Cardputer.Display.drawRect(20, 80, 200, 40, TFT_LIGHTGRAY);
        M5Cardputer.Display.setTextColor(TFT_LIGHTGRAY);
    }
    M5Cardputer.Display.setCursor(42, 90);
    M5Cardputer.Display.printf("Bluetooth");
}

void drawBattery() {
    int level = M5.Power.getBatteryLevel();
    int w = M5Cardputer.Display.width();
    int barW = 20, barH = 7;
    int barX = w - 66, barY = 8;
    int txX = w - 40, txY = 7;

    // Battery bar
    M5Cardputer.Display.drawRect(barX, barY, barW, barH, 0x4208);
    int fillW = (level * (barW - 2)) / 100;
    if (fillW > 0) {
        uint16_t fillColor = level > 20 ? 0x07E0 : TFT_RED;
        M5Cardputer.Display.fillRect(barX + 1, barY + 1, fillW, barH - 2, fillColor);
    }

    // Clear text area and draw
    M5Cardputer.Display.fillRect(txX - 2, txY, 38, 10, TFT_BLACK);
    M5Cardputer.Display.setTextColor(level > 20 ? 0x07E0 : TFT_RED);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(txX, txY);
    M5Cardputer.Display.printf("%d%%", level);
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

void drawHelpText(bool mouseMode) {
    // Clear bottom area
    M5Cardputer.Display.fillRect(0, 114, M5Cardputer.Display.width(), 21, TFT_BLACK);
    M5Cardputer.Display.setTextColor(0x4208);  // dark grey
    M5Cardputer.Display.setTextSize(1);

    if (mouseMode) {
        M5Cardputer.Display.setCursor(8, 116);
        M5Cardputer.Display.print("[Enter]: LMB  [\\]: RMB");
        M5Cardputer.Display.setCursor(8, 125);
        M5Cardputer.Display.print("[;]/[.]: scroll up/down");
    } else {
        M5Cardputer.Display.setCursor(8, 120);
        M5Cardputer.Display.print("[Fn]: switch");
    }
}

void displayMainScreen(bool usbMode, bool mouseMode, bool bluetoothStatus, bool gyroMode) {
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    drawDeviceRect(mouseMode);
    drawMouseIcon(165, 50);
    drawKeyboardIcon(42, 57);
    modeIndicator(usbMode, bluetoothStatus);
    drawBattery();
    drawHelpText(mouseMode);
}
