
#ifndef __flying__
#define __flying__

typedef struct{
	short X;
	short Y;
	
	short Width;//char ???
	
	//char Active;		
	short Active;	
	
	short Data0;
	short Data1;
	short Data2;
	short Data3;
	
	
	//char PlayerOn;
	short PlayerOn;
//	short DrawMode;	
	unsigned short Sprite;
	

	void (*Handler)(void *Flying_platform);

	
}flying_platform;

extern flying_platform* Flying_platforms;



void Handle_flying_platforms();



void Platform_handler_1(flying_platform* Platform);

void Platform_handler_2(flying_platform* Platform);

void Platform_handler_3(flying_platform* Platform);
//short Sign(short);



#endif