#ifndef _KEYBOARDHID_H_
#define _KEYBOARDHID_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Include our mock USB.h (will be found via -Imock)
#include "USB.h"
// Descriptors.h will be found in current dir, checks <USB.h> which we provide
#include "Descriptors.h"

// --- XMC MOCKS ---

// GPIO
#define P1_1 1
#define P1_0 0
#define XMC_GPIO_MODE_OUTPUT_PUSH_PULL 0

static inline void XMC_GPIO_SetMode(int pin, int mode) {}
static inline void XMC_GPIO_SetOutputHigh(int pin) { /* printf("GPIO HIGH: %d\n", pin); */ }
static inline void XMC_GPIO_SetOutputLow(int pin) { /* printf("GPIO LOW: %d\n", pin); */ }

// SCU / CLOCK
typedef struct {
    int p_div;
    int n_div;
    int k_div;
    int mode;
    int clksrc;
} XMC_SCU_CLOCK_SYSPLL_CONFIG_t;

typedef struct {
    XMC_SCU_CLOCK_SYSPLL_CONFIG_t syspll_config;
    bool enable_oschp;
    int calibration_mode;
    int fsys_clksrc;
    int fsys_clkdiv;
    int fcpu_clkdiv;
    int fccu_clkdiv;
    int fperipheral_clkdiv;
} XMC_SCU_CLOCK_CONFIG_t;

#define XMC_SCU_CLOCK_SYSPLL_MODE_NORMAL 0
#define XMC_SCU_CLOCK_SYSPLLCLKSRC_OSCHP 0
#define XMC_SCU_CLOCK_FOFI_CALIBRATION_MODE_FACTORY 0
#define XMC_SCU_CLOCK_SYSCLKSRC_PLL 0
#define XMC_SCU_CLOCK_USBCLKSRC_USBPLL 0
#define XMC_SCU_CLOCK_USB 0

static inline void XMC_SCU_CLOCK_Init(XMC_SCU_CLOCK_CONFIG_t* config) {}
static inline void XMC_SCU_CLOCK_EnableUsbPll() {}
static inline void XMC_SCU_CLOCK_StartUsbPll(int a, int b) {}
static inline void XMC_SCU_CLOCK_SetUsbClockDivider(int div) {}
static inline void XMC_SCU_CLOCK_SetUsbClockSource(int src) {}
static inline void XMC_SCU_CLOCK_EnableClock(int clk) {}
static inline void SystemCoreClockUpdate() {}

// SysTick mock
#define SystemCoreClock 120000000
static inline int SysTick_Config(uint32_t ticks) { return 0; }

// USB Init
static inline void USB_Init() {}
static inline void HID_Device_USBTask(USB_ClassInfo_HID_Device_t* interface) {}

// Global Interface
extern USB_ClassInfo_HID_Device_t Keyboard_HID_Interface;

#define HID_KEYBOARD_LED_NUMLOCK 1
#define HID_KEYBOARD_LED_CAPSLOCK 2

#endif
