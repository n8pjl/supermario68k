// C Source File
// Created 23.10.2006; 20:08:16

#include "all.h"

void Errorhandler(char Error) {

  // memset(LCD_MEM,0x00,LCD_SIZE);

  ClrScr();

  char *Msg = Texts;
  //	const char MemErrTxt[15]="Out of memory!";
  //	const char Memerr[15] = "Out of memory!";

  switch (Error) {

  case 1:
    // Msg = Texts+GameTextData.MemError;
    DrawStr(10, 30, "Out of memory!", A_REPLACE);
    break;
  case 2:
    Msg = Texts + GameTextData.GfXErr;
    break;
  case 3:
    Msg = Texts + GameTextData.LevelError;
    break;
  case 4:
    Msg = Texts + GameTextData.MapError;
    break;
  case 5:
    Msg = Texts + GameTextData.LevelsetError;
    break;
  case 6:
    Msg = Texts + GameTextData.GrayError;
    break;
  case 7:
    DrawStr(10, 30, "Textfile not found!", A_REPLACE);
    break;
  case 8:
    Msg = Texts + GameTextData.LevelSetIncompatible;
    break;
  }

  short X = 10;
  short Y = 30;
  if ((Error != 1) && (Error != 7)) {

    while (*Msg) { // loop until null character found (aka the end of the text)
      if (*Msg == 13) { // new line
        X = 10;
        Y += 16;
      } else {
        DrawChar(X, Y, *Msg, A_REPLACE);
        X += 6;
      }
      Msg++; // next...
    }

    //		DrawStr(10,30,Msg,A_REPLACE);
    //		DrawWrappedText(Msg);
  }

  // DrawStr(10,30,(Msg?Msg:"Textfile not found!"),A_REPLACE);

  /*while( (_rowread(0)) );
  while( !(_rowread(0)) );*/
  WaitKeyPress();
};
