#ifndef PASSWORD_ATTACK_H
#define PASSWORD_ATTACK_H

#include <stdint.h>
#include <stdbool.h>
#include "KeyboardHID.h"

// Public API
void AttackInit(void);
void AttackCreateReport(USB_KeyboardReport_Data_t* report);
void AttackProcessResponse(uint8_t* led_report);
void MarkAttackCompleted(void);

#endif // PASSWORD_ATTACK_H
