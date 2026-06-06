#ifndef DISPLAY_H
#define DISPLAY_H

#include <M5Cardputer.h>

void setupDisplay();
void displayWelcomeScreen();
void displaySelectionScreen(bool mouseMode);
void displayMainScreen(bool usbMode, bool mouseMode, bool bluetoothStatus, bool gyroMode);
void modeIndicator(bool usbMode, bool bluetoothStatus, bool pairingBlink = false);
void drawDeviceRect(bool reverse);
void drawGyroIndicator(bool active);

#endif
