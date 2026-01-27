#include "index_to_char.h"

// Convert index (0-84) to character
char IndexToChar(uint8_t index)
{
	// 0-25: lowercase a-z
	if (index < 26)
	{
		return 'a' + index;
	}
    
	// 26-51: uppercase A-Z
	if (index < 52)
	{
		return 'A' + (index - 26);
	}

	// 52-61: digits 0-9
	if (index < 62)
	{
		return '0' + (index - 52);
	}

	// 62-84: special characters
	static const char specialChars[] = 
    {
		'!',  // 62
		'(',  // 63
		')',  // 64
		'-',  // 65
		'_',  // 66
		'+',  // 67
		'=',  // 68
		'~',  // 69
		';',  // 70
		':',  // 71
		',',  // 72
		'.',  // 73
		'<',  // 74
		'>',  // 75
		'[',  // 76
		']',  // 77
		'{',  // 78
		'}',  // 79
		'/',  // 80
		'?',  // 81
		'&',  // 82
		'$',  // 83
		' ',  // 84
		'"'   // 85
	};

	if (index >= 62 && index <= 85)
	{
		return specialChars[index - 62];
	}

	return 0;
}
