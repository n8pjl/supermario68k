

#include "all.h"



/*char*/short doMenu(char** Menudata,short Nr_of_options){
//Renders and handles a menu with the menu items pointed to by parameter Menudata, width 12+1
//returns the selected menu item, esc
//controls: 
//		up/down: move cursor
//		enter: 	 choose currently selected menu item
//		esc:		 cansel/back/exit

	
	short Upper=0,Lower;
	
	/*char*/short X=1;
	short C;
//	void *dBufHPL, *dBufHPD;
		
	SetBg(0);
	
	
	while(1){
		//short Next = 0;
		
		//ClearGrayScreen2B(GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));//temp
		//draw bg
	
		//DrawGrayPlane(40,40,&Bg_plane,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G,drawmode_bg/*TM_GRPLC89*/,TM_G16B_ROLL/*TM_G16B*/);
		DrawBg(40,40);
		
		//draw title
		//GrayDrawStrExt(0,10,Menudata,A_REPLACE | A_CENTERED,F_8x10);
		//DrawGrayStrExt2B(30,5,Menudata,A_REPLACE|A_SHADOWED,F_8x10,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
		DrawString(30,5,Menudata[0],A_REPLACE|A_SHADOWED,F_8x10);
		
		//DrawGrayStrExt2B(24,screen_height-6,"http://tifreakware.net/lachprog/",A_REPLACE,F_4x6,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
		DrawString(24,screen_height-6,"http://tifreakware.net/lachprog/",A_REPLACE,F_4x6);
//		
		Lower = Upper + nr_of_visible_options-1;//min(Upper + nr_of_visible_options-1,Upper + Nr_of_options-1);
		C=0;
		while(C<Nr_of_options){//draw menu options
//		for(C=Upper;C<=Lower;C++){//draw menu options
			//find next string
			//Next += strlen(Menudata+Next)+1;
			
			if( (C>=Upper) && (C<=Lower) ){
			//GrayDrawStrExt(0,30+10*C,(Menudata+Next),A_REPLACE | A_CENTERED,F_6x8);
				//DrawGrayStrExt2B(30,25+15*(C-Upper),(Menudata+Next),A_NORMAL|A_SHADOWED,F_6x8,/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
				DrawString(30,25+15*(C-Upper),(Menudata[C+1]),A_NORMAL|A_SHADOWED,F_6x8);
			}
			C++;
		};				
		
		//GrayClipSprite16_SMASK_R(10,25-20+15*(X-Upper),16,Mariosprites+2*Marioanimtab[1][0],Mariosprites+2*Marioanimtab[1][0]+16,Mariomasks+Marioanimtab[1][0],/*GrayDBufGetHiddenPlane(LIGHT_PLANE)*/dBufHPL_G,/*GrayDBufGetHiddenPlane(DARK_PLANE)*/dBufHPD_G);
		DrawMarioCursor(10, 25-20+15*(X-Upper));
		//GrayDBufToggleSync(); 	//set drawbuffer as active screen
		
		GrayDBufToggleSync_SetPointers();
		
		/*while(_rowread(0));//wait until key released
		while(!_rowread(0));//wait until key pressed*/
		WaitKeyPress();
		
		
		Scankeys();
		
		Contrast_adj();
		
		if(Keystate.Esc){
			return 0;
		};
		if(Keystate.Enter || Keystate.Jump){
			return X;
		};
		
		if(Keystate.Up){
			X--;
			if(X<1)
				X=Nr_of_options;
						
		};
		if(Keystate.Down){
			X++;
			if(X>Nr_of_options)
				X=1;
		};
		
		if(X<=Upper)
			Upper=X-1;
		if(X>Lower)
			Upper=max(X-nr_of_visible_options,0);		
				
	};
	
	return 0;
	
};


short Menus(){
	SYM_ENTRY* SymPtr;
	short C=0;
	short Offset = 0;
	/*char*/short Res;
	char Menu=mainMenu;
	
	char* TempBuffer = Fg_plane.p.big_vscreen;
	
	/*strcpy(Fg_plane.p.big_vscreen , "Levelset:");
	Offset = 10;*/
	Offset = StringCopy(TempBuffer/*Fg_plane.p.big_vscreen*/ , "Levelset:");
	
	//find all levelsets
	SymPtr = FFindFirst (FO_RECURSE, "MLST", "");//find all *.LSET files

	while (SymPtr){
		  
	  if (!memcmp(HToESI(SymPtr->handle)-(4+2),(char [4+3]){0,'M','L','S','T',0,OTH_TAG},4+3)){//if mlev
			
			/*strcpy(Fg_plane.p.big_vscreen+Offset , HeapDeref (SymPtr->handle)+2 +sizeof(levelsetdata) );
			Offset += strlen(Fg_plane.p.big_vscreen+Offset)+1;*/
			Offset += StringCopy(TempBuffer/*Fg_plane.p.big_vscreen*/+Offset , HeapDeref (SymPtr->handle)+2 +sizeof(levelsetdata));
			//strcpy 
			StringCopy(TempBuffer/*Fg_plane.p.big_vscreen*/+512+9*C,SymPtr->name);
			
			//store list of folders...
			
			StringCopy( TempBuffer+1024+9*C , SymFindFolderName() );
			
			C++;
	  };
	  SymPtr = FFindNext ();
	};
	
	//char Levelset[9];
	
	//Error: no levelset found possible
	
	if(C>1){//more than one levelset detected
		
		Res = doMenu(TempBuffer/*Fg_plane.p.big_vscreen*/,C);//let the user choose which to use
	
		//strcpy(Levelset,Fg_plane.p.big_vscreen+512+(C-1)*9);
		StringCopy(Levelsetfile,TempBuffer/*Fg_plane.p.big_vscreen*/+512+(Res-1)*9);
		
	}
	else{//only one levelset found
		if(C==0){
		//strcpy(Levelset,Fg_plane.p.big_vscreen+512);
			return 5;
		}
		else{
			StringCopy(Levelsetfile,TempBuffer/*Fg_plane.p.big_vscreen*/+512);
			Res=1;
		}
	};
	// TempBuffer+1024+9*C
	if( !FolderCur (SYMSTR(TempBuffer+1024+9*(Res-1)), TRUE) ){
//		ST_folder (TempBuffer+1024+9*(Res-1));
		return 0;
	}


		
	HANDLE Temp; 
	//Load levelsetdata!

	Temp = File_get_pointer_and_lock(Levelsetfile/*,Levelsetfile_sym*/);
	
	memcpy( &Levelsetdata, HeapDeref (Temp)+2, sizeof(levelsetdata) );
	
	memcpy( &Commonfilename, HeapDeref (Temp)+2+sizeof(levelsetdata)+21+9*Levelsetdata.Commonfile, 9 );
	
	if(Levelsetdata.Compatibility>compatibility){//If chosen levelset is designed for newer versions of the game...
		return 8;//Levelset not compatible. New version of the game must be installed
	}
	
	//Error: Memory possible
	if( !(Filenames = malloc(9*(Levelsetdata.Nr_of_files))) ){
		return 1;
	};
	
	memcpy( Filenames, HeapDeref (Temp)+2+sizeof(levelsetdata)+21, 9*Levelsetdata.Nr_of_files );
	
	//HeapUnlock(	SymPtr->handle );
	HeapUnlock(	Temp );
	
	
	//New: V 1.01 Added error check
	if(Show_titlescreen()){
		return 3;
	};
	//End of new
	
	while(!Exit){
		
		while(Menu==mainMenu){
			
			Res = doMenu(GameTextData.main_menu,4);
			
			switch(Res){

				case 1://new game
					
					RunGame(-1);
					if(ErrorCode)
						return ErrorCode;
					
				break;
				case 2://load game
					Menu = load;
				break;
				case 3://options
					Key_configure();
				break;
				/*case 4://help
				
				break;*/
				case 0://esc
				case 4://quit
					Exit = 1;
					Menu = none;
				break;
				
			}

		}//end main menu

		
		/*while(Menu==options){//to come
			
			Res = doMenu(Texts+GameTextData.Options,3);
			
			switch(Res){

				case 1://statusbar
				
				break;
				
				case 2://controls
				
				break;
				
				case 0://esc
				case 3://back
					Menu=main;
				break;
		
			};
			
		};//end controls menu
		*/
		while(Menu==load){
			
			/*strcpy(Fg_plane.p.big_vscreen,Texts+GameTextData.Load);//Title
			C = Offset = strlen(Fg_plane.p.big_vscreen)+1;*/
			
//			C = Offset = StringCopy(TempBuffer/*Fg_plane.p.big_vscreen*/,Texts+GameTextData.Load);
			
			Paste_saveslot_text(TempBuffer/*Fg_plane.p.big_vscreen*/,GameTextData.load_menu);//Offset);
			Res = doMenu(TempBuffer/*Fg_plane.p.big_vscreen*/,3);

			if( (Res>=1) && (Res<=3) ){
				if(Levelsetdata.Savegames[Res-1]){//Only load if saveslot is used!
					RunGame(Res-1);
					if(ErrorCode)
						return ErrorCode;
					Menu=mainMenu;//Go to main menu when finiched/exit
					Exit = 0;
				}
					
			}
			else{
				Menu=mainMenu;
			}
			
		};//end controls menu
		
	}
	
	return 0;
};

void Paste_saveslot_text(char** Buffer,const char* const* Text){
	char TempBuffer[64];

	//Dravs saveslot text (Title + save 1, save 2, <empty>)
	//Languages fully supported
	short Offset;//The position in the output buffer to write to
	short L;//Position in the input buffer to fetch next string
	short C;
	
	char* OutBuffer = Fg_mask.p.big_vscreen;
	
	Offset = L = StringCopy(Buffer[0],Text[0]);//title
	
	L += StringCopy(TempBuffer,Text[1]);//save 1
	StringCopy(TempBuffer+30,Text[2]);//Empty slot txt

	for(C=0;C<3;C++){
		Buffer[C+1] = OutBuffer + Offset;

		if(Levelsetdata.Savegames[C]){

			Offset += StringCopy(OutBuffer+Offset,TempBuffer);
			*(Buffer+Offset-2) += C;
			
		}
		else{

			Offset += StringCopy(OutBuffer+Offset,TempBuffer+30);
			
		}
	}
			
}
