#include "report_utils.h"
#include "german_keyboardCodes.h"
#include "index_to_char.h"
#include "char_code_german.h"

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
