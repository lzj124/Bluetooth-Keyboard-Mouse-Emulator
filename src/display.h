#ifndef DISPLAY_H
#define DISPLAY_H

#include <M5Cardputer.h>

void setupDisplay();
void displayWelcomeScreen();
void displayMainScreen(bool usbMode, bool mouseMode, bool bluetoothStatus, bool gyroMode);
void modeIndicator(bool usbMode, bool bluetoothStatus);
void drawDeviceRect(bool reverse);
void drawBattery();
void drawHelpText(bool mouseMode, bool toggleMode);

// Settings page
#define SETTING_SENSITIVITY 0
#define SETTING_TOGGLE_MODE 1
#define SETTING_USB_MODE    2
#define SETTING_COUNT       3

void displaySettingsScreen(int selected, int sensitivity, bool toggleMode, bool usbMode);

#endif
