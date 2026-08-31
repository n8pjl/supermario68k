// C Source File
// Created 23.10.2006; 20:08:16

#include "compat/graph.h"
#include "scankeys.h"
#include "text.h"
#include <stdlib.h>

void Errorhandler(char Error)
{
	ClrScr();

	const char *msg_id;

	switch (Error) {
	case 1:
		// Msg = Texts+GameTextData.MemError;
		DrawStr(10, 30, "Out of memory!", A_REPLACE);
		break;
	case 2:
		msg_id = "GfXErr";
		break;
	case 3:
		msg_id = "LevelError";
		break;
	case 4:
		msg_id = "MapError";
		break;
	case 5:
		msg_id = "LevelsetError";
		break;
	case 6:
		msg_id = "GrayError";
		break;
	case 7:
		DrawStr(10, 30, "Textfile not found!", A_REPLACE);
		break;
	case 8:
		msg_id = "LevelSetIncompatible";
		break;
	}

	int16_t X = 10;
	int16_t Y = 30;
	if ((Error != 1) && (Error != 7)) {
		char *msg = get_string_locale(msg_id);

		while (*msg_id) { // loop until null character found (aka the end of the text)
			if (*msg_id == 13) { // new line
				X = 10;
				Y += 16;
			} else {
				DrawChar(X, Y, *msg_id, A_REPLACE);
				X += 6;
			}
			msg_id++; // next...
		}

		free(msg);
	}

	WaitKeyPress();
};
