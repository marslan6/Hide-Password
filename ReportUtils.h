#ifndef REPORTUTILS_H
#define REPORTUTILS_H

#include "KeyboardHID.h"
#include <stdint.h>

void ClearReport(USB_KeyboardReport_Data_t *report);
void SendEnterKey(USB_KeyboardReport_Data_t *report);
void SendCharacter(uint8_t charIndex, USB_KeyboardReport_Data_t *report);

#endif // REPORTUTILS_H
