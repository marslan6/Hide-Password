#ifndef _USB_H_
#define _USB_H_

#include <stdint.h>
#include <stdbool.h>

// Mock LUFA types
typedef struct {
    uint8_t Modifier;
    uint8_t Reserved;
    uint8_t KeyCode[6];
} USB_KeyboardReport_Data_t;

typedef struct {
    int dummy;
} USB_ClassInfo_HID_Device_t;

// Descriptor mocks
typedef struct { int d; } USB_Descriptor_Configuration_Header_t;
typedef struct { int d; } USB_Descriptor_Interface_t;
typedef struct { int d; } USB_HID_Descriptor_HID_t;
typedef struct { int d; } USB_Descriptor_Endpoint_t;

// LUFA / HID Constants
#define ENDPOINT_DIR_IN 1
#define ATTR_WARN_UNUSED_RESULT
#define ATTR_NON_NULL_PTR_ARG(x)

#define HID_KEYBOARD_SC_CAPS_LOCK 0x39
#define HID_KEYBOARD_MODIFIER_LEFTSHIFT (1 << 1)
#define HID_KEYBOARD_MODIFIER_RIGHTALT  (1 << 6)

#endif
