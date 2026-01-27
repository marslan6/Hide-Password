#include "report_utils.h"
#include "german_keyboardCodes.h"

// External function declarations
extern char IndexToChar(uint8_t index);
extern uint8_t GetCharCodeInGerman(char c, uint8_t *modifier);

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
