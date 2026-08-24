#ifndef __scankeys__
#define __scankeys__






typedef struct {
	short Enter;
	short Esc;
	short Jump;
	short Down;
	short Left;
	short Right;
	short Up;
	short Run;
	short Plus;
	short Minus;
//	short Fire;//not in use any more
		
//	short Keydetect;//not neccessarry
}keystate;

extern keystate Keystate,Previous_keystate;

void WaitKeyReleased();

void WaitKeyPress();

void Scankeys();






#endif