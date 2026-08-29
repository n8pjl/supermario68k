#include "custom.h"
#include "compat/extgraph.h"
#include "compat/graph.h"
#include "compat/tios.h"
#include "control.h"
#include "gfx.h"
#include "level.h"
#include "menus.h"
#include "render.h"
#include "savegame.h"
#include "scankeys.h"
#include <stdint.h>
#include <string.h>

struct settings Settings;

inline void Custom() {
  if (Keystate.esc) {
    // handle midgame menu

    char OldBg = Leveldata.Background;
    int16_t OldBg_offset = Leveldata.Bg_offset;

    /*char*/ int16_t Res = doMenu(Texts + GameTextData.MidGameLevel, 2);

    SetBg(OldBg /*,Bg_file_sym->handle*/);
    Leveldata.Bg_offset = OldBg_offset;

    if (Res == 2) {
      Exit = 9;
    }

    // Keystate.Esc = 0;
    WaitKeyReleased(); // while( (_rowread(0)) );

    // Exit = 1;//temporary
    // DrawStr(0,20,"wee!!!",A_NORMAL);//used as indicator during tests
  };
}

// New V 1.03: Configureable keys

#define nr_of_config_keys 2

void Key_configure() {
  // Configureable keys:
  // Jump / Fly button
  // Run / Fire button

  // Available keys:
  // Ti 89 / Ti 89 Titanium:
  // [2nd] [Shift] [Diamond] [Alpha]

  // Default:
  // Jump / Fly button: [2nd]
  // Run / Fire button: [Diamond]

#ifdef PRODUCE_TI89_CODE
  const char *Keys[4] = {"[2nd]", "[Shift]", "[Diamond]", "[Alpha]"};
#endif
#ifdef PRODUCE_TI92PLUS_CODE
  const char *Keys[4] = {"[Hand]", "[F1]", "[F2]", "[F5]"};
#endif
#ifdef PRODUCE_V200_CODE
  const char *Keys[4] = {"[Hand]", "[Q]", "[F1]", "[W]"};
#endif

  SetBg(0);

  int16_t Curr = 0;

  int16_t Back = 0;

  while (!Back) {

    // DrawGrayPlane(40,40,&Bg_plane,dBufHPL_G,dBufHPD_G,drawmode_bg,TM_G16B_ROLL);
    DrawBg(40, 40);

    char *Txt = Texts + GameTextData.KeyConfigTexts;

    // DrawGrayStrExt2B(20,5,Txt,A_NORMAL|A_SHADOWED,F_8x10,dBufHPL_G,dBufHPD_G);
    DrawString(20, 5, Txt, A_NORMAL | A_SHADOWED, F_8x10);

    int16_t C, Next = 0;
    for (C = 0; C < 2; C++) {
      Txt += strlen(Txt) + 1;
      // DrawGrayStrExt2B(30,25+C*16,Txt,A_NORMAL|A_SHADOWED,F_6x8,dBufHPL_G,dBufHPD_G);
      DrawString(30, 25 + C * 16, Txt, A_NORMAL | A_SHADOWED, F_6x8);
    }

    // Draw current selected keys

    for (C = 0; C < 2; C++) {
      // DrawGrayStrExt2B(100,25+C*16,Keys[Settings.Keys[C]],A_NORMAL|A_SHADOWED,F_6x8,dBufHPL_G,dBufHPD_G);
      DrawString(100, 25 + C * 16, Keys[Settings.Keys[C]],
                 A_NORMAL | A_SHADOWED, F_6x8);
    };

    // GrayClipSprite16_SMASK_R(10,23+16*Curr,16,Mariosprites+2*Marioanimtab[1][0],Mariosprites+2*Marioanimtab[1][0]+16,Mariomasks+Marioanimtab[1][0],dBufHPL_G,dBufHPD_G);
    DrawMarioCursor(10, 23 + 16 * Curr);

    GrayDBufToggleSync_SetPointers();

    /*while(_rowread(0));//wait until key released
    while(!_rowread(0));//wait until key pressed*/
    WaitKeyPress();

    ScanKeys();

    if (Keystate.down) {
      if ((++Curr) > (nr_of_config_keys - 1)) {
        Curr = 0;
      }
    };
    if (Keystate.up) {
      if ((--Curr) < 0) {
        Curr = (nr_of_config_keys - 1);
      }
    };

    do {

      if (Keystate.left) {
        if ((--Settings.Keys[Curr]) < 0) {
          Settings.Keys[Curr] = 3;
        }
      };
      if (Keystate.right) {
        if ((++Settings.Keys[Curr]) > 3) {
          Settings.Keys[Curr] = 0;
        }
      };

    } while (Settings.Keys[0] ==
             Settings.Keys[1]); // Prevent assigning the same key to both

    if (Keystate.enter || Keystate.esc) {
      Back = 1;
    }
  }

  FILES fsPtr;

  if (FOpen("mario\\maconfig", &fsPtr, FM_WRITE, "MCFG") != FS_OK) {
    return;
  }
  // write buffer to file
  if (FWrite(&Settings, sizeof(struct settings), &fsPtr) !=
      FS_OK) { // write everythiong to file
    return;
  }
  // close file
  FClose(&fsPtr);
};

void Load_settings() {

  HANDLE Conf;

#ifdef PRODUCE_TI89_CODE
  const int16_t DefJump = 0, DefRun = 2;
#endif
#ifdef PRODUCE_TI92PLUS_CODE
  const int16_t DefJump = 1, DefRun = 2;
#endif
#ifdef PRODUCE_V200_CODE
  const int16_t DefJump = 0, DefRun = 1;
#endif

  Settings.Keys[0] = DefJump;
  Settings.Keys[1] = DefRun;

  if ((Conf = File_get_pointer_and_lock("mario\\maconfig")) != H_NULL) {
    const uint8_t *Raw = HeapDeref(Conf);
    // Blobs carry a 2-byte big-endian payload size ahead of the data.
    if (Raw && ((Raw[0] << 8) | Raw[1]) >= (int16_t)sizeof(struct settings)) {
      memcpy(&Settings, Raw + 2, sizeof(struct settings));
    }
  }

  // The slots are indices into a 4-entry per-model key table that Scankeys()
  // reads without bounds-checking. Reject a corrupt or foreign config rather
  // than let it point jump/run at garbage.
  for (int16_t C = 0; C < 2; C++) {
    if (Settings.Keys[C] < 0 || Settings.Keys[C] > 3) {
      Settings.Keys[0] = DefJump;
      Settings.Keys[1] = DefRun;
      break;
    }
  }
  if (Settings.Keys[0] == Settings.Keys[1]) {
    Settings.Keys[0] = DefJump;
    Settings.Keys[1] = DefRun;
  }
};

// End of new
