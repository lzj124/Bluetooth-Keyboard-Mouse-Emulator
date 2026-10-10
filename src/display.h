#ifndef DISPLAY_H
#define DISPLAY_H

#include <M5Cardputer.h>
#include "btHosts.h"

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
#define SETTING_HOSTS       3
#define SETTING_COUNT       4

void displaySettingsScreen(int selected, int sensitivity, bool toggleMode, bool usbMode);

// Hosts 子页（3 个槽 + Disconnect + Forget all = 5 行）
#define HOSTS_ROW_DISCONNECT (BTHOST_SLOTS)
#define HOSTS_ROW_FORGET_ALL (BTHOST_SLOTS + 1)
#define HOSTS_ROW_COUNT      (BTHOST_SLOTS + 2)

void displayHostsScreen(int selected);

#endif
