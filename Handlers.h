#ifndef HANDLERS_H
#define HANDLERS_H

#include "KeyboardHID.h"
#include <stdbool.h>

void HandleCharacterRelease(USB_KeyboardReport_Data_t* report);
void HandleEnterKeyPress(USB_KeyboardReport_Data_t* report);
void HandlePasswordCharSend(USB_KeyboardReport_Data_t* report, bool* isReleased);
void HandlePasswordInput(USB_KeyboardReport_Data_t* report, bool* isReleased);
void HandleCapsLockToggle(USB_KeyboardReport_Data_t* report, bool* capsLockPressed);
void HandleNameOutput(USB_KeyboardReport_Data_t* report);

#endif // HANDLERS_H
