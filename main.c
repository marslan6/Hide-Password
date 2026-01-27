#include "KeyboardHID.h"
#include "german_keyboardCodes.h"
#include "Handlers.h"
#include "ReportUtils.h"
#include "IndexToChar.h"
#include "CharCodeGerman.h"

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