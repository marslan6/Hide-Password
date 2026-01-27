#ifndef PASSWORD_ATTACK_H
#define PASSWORD_ATTACK_H

#include <stdint.h>
#include <stdbool.h>
#include "KeyboardHID.h"

// Function prototypes
void attack_init(void);
void attack_create_report(USB_KeyboardReport_Data_t* report);
void attack_process_response(uint8_t* led_report);
void findBestTimeAndExtractPasswordChar();
void sendResolvedChars(USB_KeyboardReport_Data_t* report);
void sendUnsolvedCurrentChar(USB_KeyboardReport_Data_t* report);
void sendEnterKey(USB_KeyboardReport_Data_t* report);
void mark_attack_completed();

// External access to password_set_of_chars if needed
extern const char password_set_of_chars[];
extern uint8_t password_set_of_chars_len;

#endif // PASSWORD_ATTACK_H
