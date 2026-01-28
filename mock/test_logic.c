#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Include our mocks
// The compilation command will use -Imock to prioritize our headers.

// Define 'main' as 'app_main' so we don't have symbol collision
#define main app_main

// We need to define the global interface variable that main.c uses expects
#include "KeyboardHID.h"
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface;

// Now include the actual application code
#include "main.c"

// Undefine main so we can write our own
#undef main

// Helper to decode HID keycode to char
char hidToChar(uint8_t hidCode, uint8_t modifier) {
    bool shifted = (modifier & 0x02) != 0;
    
    // Simple mapping for letters and numbers based on german_keyboardCodes.h/US standard
    // Note: The logic in main.c uses german_keyboardCodes.h to ENCODE.
    // We want to decode to see what it's sending.
    // Since we have german_keyboardCodes.h available...
    
    // Let's just do a reverse lookup on germanKeymap if possible, or build a mini table.
    // Or just print the HID ID if complex.
    
    if (hidCode == 0) return 0; // No key
    if (hidCode == GERMAN_KEYBOARD_SC_ENTER) return '\n';
    
    // Scan the map
    for (size_t i = 0; i < GERMAN_KEYMAP_SIZE; ++i) {
        if (germanKeymap[i].hidValue == hidCode) {
            // Found a match. The 'character' field is a string.
            // If it's a shifted char, we might need to be careful?
            // The mapping table has Entries like "A" mapping to SC_A (0x04).
            // But "a" also maps to SC_A.
            // main.c logic: if char is upper, it sets modifier=0x02.
            
            const char* str = germanKeymap[i].character;
            // Check if this entry matches our shift state expectations
            // This is loose "decoding" because the map isn't 1:1 unique (a vs A).
            // But for logging, "a" or "A" is fine as long as we know what happened.
            if (strlen(str) == 1) return str[0];
            if (strcmp(str, "enter") == 0) return '\n';
        }
    }
    return '?';
}

int main() {
    printf("Starting main.c logic test...\n");

    // Initialize report structures
    USB_ClassInfo_HID_Device_t hidInfo;
    USB_KeyboardReport_Data_t report;
    uint8_t reportID = 0;
    uint16_t reportSize = 0;

    // Run the loop for some iterations to see what it types
    // The device logic sends keys in CALLBACK_HID_Device_CreateHIDReport
    
    printf("--- Output Sequence ---\n");
    
    // Simulate 500 polls (should be enough to see some passwords)
    for (int i = 0; i < 500; i++) {
        memset(&report, 0, sizeof(report));
        bool res = CALLBACK_HID_Device_CreateHIDReport(
            &hidInfo,
            &reportID,
            0,
            &report,
            &reportSize
        );

        if (res && report.KeyCode[0] != 0) {
           char c = hidToChar(report.KeyCode[0], report.Modifier);
           if (c == '\n') {
               printf("\n[ENTER]\n");
           } else if (c != 0) {
               printf("%c", c);
           }
        }
    }
    printf("\n--- End Sequence ---\n");

    return 0;
}
