#include "char_code_german.h"
#include "german_keyboardCodes.h"
#include "KeyboardHID.h"

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
