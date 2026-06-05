#ifndef USBHID_H
#define USBHID_H

#include "USBHIDMouse.h"
#include "USBHIDKeyboard.h"
#include <M5Cardputer.h>

extern float gyroX;
extern float gyroZ;

void usbMouse(bool gyroMode);
void usbKeyboard();
void handleUsbMode(bool mouseMode, bool gyroMode);

#endif
