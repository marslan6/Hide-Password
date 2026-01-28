#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// --- Mocks ---
#define main app_main
#include "KeyboardHID.h"
#include "../main.c"
#undef main

// We need to inject our mocks into the compilation. 
// We will compile test_attack.c with -Imock

USB_ClassInfo_HID_Device_t Keyboard_HID_Interface;

extern void HID_Device_USBTask(USB_ClassInfo_HID_Device_t* interface);
extern bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, uint8_t* const ReportID, const uint8_t ReportType, void* ReportData, uint16_t* const ReportSize);
extern void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, const uint8_t ReportID, const uint8_t ReportType, const void* ReportData, const uint16_t ReportSize);

// Expose variables from main.c if accessible, or we rely on observing HID output
// The main.c has 'discoveredPasswordBuffer' which we might want to check
extern char discoveredPasswordBuffer[20];
extern uint8_t extractedPasswordLength;

// -- Simulation Logic --
const char* SECRET_PASSWORD = "secret"; // The password the host "knows"
#define BASE_DELAY 100 // Ticks
#define MATCH_DELAY 1000 // Ticks per char

// Helper to decode HID (Simplified mapping for debugging)
char hidToChar(uint8_t hidCode, uint8_t modifier) 
{
    // a-z
    if (hidCode >= 0x04 && hidCode <= 0x1D) return 'a' + (hidCode - 0x04);
    // 0-9 (standard row, not numpad)
    if (hidCode >= 0x1E && hidCode <= 0x27) return "1234567890"[hidCode - 0x1E];
    
    if (hidCode == 0x28) return '\n'; // ENTER
    if (hidCode == 0x2C) return ' '; // SPACE
    // Map some common symbols if easy, otherwise return a placeholder that isn't ignored
    return '.'; // Return dot for unmapped chars so we see *something*
}

extern uint8_t testingCharacterIndex;
extern bool readyForNextCharacter;

int main() 
{
    printf("Starting HIDE_PASSWORD Attack Simulation...\n");

    printf("Target Password: %s\n", SECRET_PASSWORD);

    USB_ClassInfo_HID_Device_t hidInfo;
    USB_KeyboardReport_Data_t report;
    uint8_t reportID = 0;
    uint16_t reportSize = 0;
    
    char inputBuffer[100];
    int inputPos = 0;
    
    // Simulate loop
    // main.c relies on 'readyForNextCharacter' flag being set by NumLock Update
    // And 'system_ticks' measuring time
    
    // We need to access 'system_ticks' to simulate time passing!
    // It is 'volatile uint32_t system_ticks' in main.c
    // We need to declare it extern here to modify it.
    extern volatile uint32_t system_ticks;
    
    // Initial state: Send NumLock to start the process (as per ProcessHIDReport logic)
    // "if (*report & HID_KEYBOARD_LED_NUMLOCK) ... readyForNextCharacter = true"
    
    uint8_t ledReport = HID_KEYBOARD_LED_NUMLOCK;
    CALLBACK_HID_Device_ProcessHIDReport(&hidInfo, 0, 0, &ledReport, 1);
    
    int max_iterations = 200000;
    int iterations = 0;
    bool pendingVerification = false;
    
    while (iterations++ < max_iterations) 
    {
        memset(&report, 0, sizeof(report));
        bool res = CALLBACK_HID_Device_CreateHIDReport(&hidInfo, &reportID, 0, &report, &reportSize);
        
        if (res && report.KeyCode[0] != 0) 
        {
            char c = hidToChar(report.KeyCode[0], report.Modifier);
            
            if (c == '\n') 
            {
                // Device submitted a guess
                inputBuffer[inputPos] = '\0';
                printf("Device guessed: %s (len: %ld). State: Idx=%d, Ready=%d\n", 
                       inputBuffer, strlen(inputBuffer), testingCharacterIndex, readyForNextCharacter);
                
                // We got ENTER. The device logic starts timer now.
                // We should wait for the device to RELEASE the key before we send the response,
                // or at least ensure we don't send it too early if the device logic overwrites the flag.
                // The device resets readyForNextCharacter = false in HandleCharacterRelease.
                // useful to wait for release.
                pendingVerification = true;

            } 
            else if (c != 0 && c != '?') 
            {
                if (inputBuffer[0] == 0 && inputPos > 0) inputPos = 0; // Reset if new line started weirdly? No.
                if (inputPos < 99) 
                {
                    inputBuffer[inputPos++] = c;
                }
            }
        } 
        else 
        {
            // Report Empty -> Key Released
            if (pendingVerification) 
            {
                // Calculate simulated processing time
                int matchCount = 0;
                size_t len = strlen(SECRET_PASSWORD);
                size_t inputLen = strlen(inputBuffer);
                
                for(size_t i=0; i<len && i<inputLen; i++) 
                {
                    if (inputBuffer[i] == SECRET_PASSWORD[i]) 
                    {
                        matchCount++;
                    } 
                    else 
                    {
                        break;
                    }
                }
                
                if (strcmp(inputBuffer, SECRET_PASSWORD) == 0 && inputLen == len) 
                {
                    printf("\nSUCCESS: Device cracked the password: '%s'\n", inputBuffer);
                    return 0;
                }
                
                // Simulate Time Passing
                uint32_t simulatedDelay = BASE_DELAY + (matchCount * MATCH_DELAY);
                printf("Simulating delay: %d ticks (Match: %d)\n", simulatedDelay, matchCount);
                system_ticks += simulatedDelay;
                
                // Send Response (NumLock)
                ledReport = HID_KEYBOARD_LED_NUMLOCK;
                CALLBACK_HID_Device_ProcessHIDReport(&hidInfo, 0, 0, &ledReport, 1);
                
                // Reset buffer for next guess
                inputPos = 0;
                pendingVerification = false;
            }
        }
    }
    
    printf("TIMEOUT: Device verify failed to crack password in %d iterations.\n", iterations);
    return 1;
}
