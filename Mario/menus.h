
#ifndef __menus__
#define __menus__

enum Menus {none=0,main=1,load=2,options=3
						
						
						
					 };

/*char*/short doMenu(char* Menudata,short Nr_of_options);

short Menus();

void Paste_saveslot_text(char* Buffer,char* Text);//short L);


#define nr_of_visible_options 4












#endif