

#include "all.h"

void Savegame(char Slot) {
  short C;
  short Size;
  //	SYM_ENTRY *Levelsetfile_sym;
  HANDLE Temp;

  //	Temp = Levelsetfile_get_pointer_and_lock();
  SYM_ENTRY *Levelfile_sym;
  Temp = File_get_pointer_and_lock(Levelsetfile /*,Levelfile_sym*/);
  // copy file content...

  // levelsetdata, title and filenames

  Size = sizeof(levelsetdata) + 21 + 9 * (Levelsetdata.Nr_of_files + 1);

  char *TempBuffer = Fg_plane.p.big_vscreen;
  memcpy(TempBuffer /*Fg_plane.p.big_vscreen*/, HeapDeref(Temp) + 2, Size);

  // copy 2 of 3 saveslots, excluding the one to be used

  for (C = 0; C < 3; C++) {

    if ((C != Slot) && (Levelsetdata.Savegames[C] != 0)) {

      Size += Size % 2;
      memcpy(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size,
             HeapDeref(Temp) + 2 + Levelsetdata.Savegames[C],
             Levelsetdata.Savegame_size[C]);
      // update saveslot pointer
      ((levelsetdata *)TempBuffer /*Fg_plane.p.big_vscreen*/)->Savegames[C] =
          Levelsetdata.Savegames[C] = Size;
      // Levelsetdata.Savegames[C] = Size;

      Size += Levelsetdata.Savegame_size[C];
    };

    // Levelsetdata.Savegames[C];
    // Levelsetdata.Savegame_size[C];
  };

  //	HeapUnlock(	Levelsetfile_sym->handle );
  HeapUnlock(Temp);

  // generate savegame :
  // save player

  Size += Size % 2; // temp

  // update saveslot pointer
  // Levelsetdata.Savegames[(short)Slot] = Size;
  ((levelsetdata *)TempBuffer /*Fg_plane.p.big_vscreen*/)
      ->Savegames[(short)Slot] = Levelsetdata.Savegames[(short)Slot] = Size;

  SavePlayer.MapBorderLeft = Map_data.Border_left;
  SavePlayer.MapBorderRight = Map_data.Border_right;
  SavePlayer.MapColor = Map_data.Color;

  memcpy(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size, &SavePlayer,
         sizeof(saveplayer));

  Size += sizeof(saveplayer);

  // Map_data.Height*Map_data.Width+sizeof(map_trigger)*Map_data.Nr_of_triggers+sizeof(map_object)*(Map_data.Nr_of_objects)
  // ))

  StringCopy(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size, Levelfilename);

  *(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size + 9) =
      Levelsetdata.CurrentWorld;

  //	Size += 10;

  // save cardgame
  for (C = 0; C < 18; C++) {
    *(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size + C + 10) = *(CardGame + C);
  }
  Size += (18 + 10);

  short MapSize = Map_data.Height * Map_data.Width +
                  sizeof(map_trigger) * Map_data.Nr_of_triggers +
                  sizeof(map_object) * Map_data.Nr_of_objects;

  char *TempBuffer2 = Fg_mask.p.big_vscreen;
  memcpy(TempBuffer2 /*Fg_mask.p.big_vscreen*/, Map,
         MapSize); // copy possibly modified map with objects and triggers

  short OldPCX = SavePlayer.PrevCompX;
  short OldPCY = SavePlayer.PrevCompY;
  short OldMapX = SavePlayer.MapX;
  short OldMapY = SavePlayer.MapY;
  short OldL = SavePlayer.L;
  short OldFgX = FgX;
  short OldFgY = FgY;

  // New: V 1.01 Added check whether or not warp zone is to be loaded. Bugfix
  Load_map((Levelsetdata.CurrentWorld == Levelsetdata.Commonfile
                ? Commonfilename
                : Levelfilename)); // load original map
  // End of new

  Map_data.Color = SavePlayer.MapColor;

  // calculate deviation
  for (C = 0; C < MapSize; C++) {
    *(TempBuffer2 /*Fg_mask.p.big_vscreen*/ + C) -= *(Map + C);
  }

  SavePlayer.PrevCompX = OldPCX;
  SavePlayer.PrevCompY = OldPCY;
  SavePlayer.MapX = OldMapX;
  SavePlayer.MapY = OldMapY;
  SavePlayer.L = OldL;
  FgX = OldFgX;
  FgY = OldFgY;

  // RLE compress map deviation
  // unsigned short RLECompress(unsigned char *output,unsigned char
  // *input,unsigned short length);

  short RLESize = RLECompress(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size,
                              TempBuffer2 /*Fg_mask.p.big_vscreen*/, MapSize);
  // short SlotSize = sizeof(saveplayer)+10+RLESize;
  for (C = 0; C < MapSize; C++) { // Undo loading of original map
    *(Map + C) += *(TempBuffer2 /*Fg_mask.p.big_vscreen*/ + C);
  };

  Map_data.Border_left = SavePlayer.MapBorderLeft;
  Map_data.Border_right = SavePlayer.MapBorderRight;

  short Slotsize = sizeof(saveplayer) + 10 + 18 + RLESize;
  Size += RLESize;

  ((levelsetdata *)TempBuffer /*Fg_plane.p.big_vscreen*/)
      ->Savegame_size[(short)Slot] = Levelsetdata.Savegame_size[(short)Slot] =
      Slotsize;
  // Levelsetdata.Savegame_size[(short)Slot] = Slotsize;
  //((levelsetdata*)Fg_plane.p.big_vscreen)->Savegame_size[(short)Slot] =
  // Size-((levelsetdata*)Fg_plane.p.big_vscreen)->Savegames[(short)Slot];
  //((levelsetdata*)Fg_plane.p.big_vscreen)->Savegame_size[(short)Slot] =
  // Levelsetdata.Savegame_size[(short)Slot] = SlotSize;

  // memcpy(Fg_plane.p.big_vscreen,&Levelsetdata,sizeof(levelsetdata));

  // open levelsetfile in writemode

  FILES fsPtr;

  // check if archived...

  // New: V 1.01 Modified the file saving to automaticly archive save game file,
  // doing a garbage collection if neccessarry
  short Tries = 0;

  while (Tries < 2) { // 0 or 1 try

    if (EM_findEmptySlot(Size +
                         100)) { // large enough empty slot in archive mem?
      // save:
      // Unarchive
      if (!EM_moveSymFromExtMem(SYMSTR(Levelsetfile), HS_NULL)) {
        goto failure; // while(1);
      }
      // Open the file
      if (FOpen(Levelsetfile, &fsPtr, FM_WRITE, "MLST") != FS_OK) {
        goto failure; // while(1);
      }
      // write buffer to file
      if (FWrite(TempBuffer /*Fg_plane.p.big_vscreen*/, Size, &fsPtr) !=
          FS_OK) { // write everythiong to file
        goto failure;
      }
      // close file
      FClose(&fsPtr);

      // archive
      if (!EM_moveSymToExtMem(SYMSTR(Levelsetfile), HS_NULL)) {
        goto failure; // while(1);
      }
      break; // break out of while
    } else { // No free flash mem blocks that are large enough
      // Garbage collect, no dialog

      if (Tries >
          0) // tried garbage collection already, no point in trying twice
        goto failure;

      if (EM_GC(FALSE)) {
        Load_gfx_from_file(); // Retrieve pointers to GFX files, might be
                              // invalid after garbage collection
      };
      Tries++;
    }
  }
  // End of new

  /*
  //unarchive
  if( !EM_moveSymFromExtMem ( SYMSTR(Levelsetfile), HS_NULL ) ){
          goto failure;//while(1);
  }

  if(FOpen (Levelsetfile, &fsPtr, FM_WRITE, "MLST")!=FS_OK){
          goto failure;//while(1);
  }

  //write buffer to file
  if(FWrite (TempBuffer, Size, &fsPtr)!=FS_OK){//write everythiong to file
          goto failure;
  }


  FClose (&fsPtr);//close file
  */
  // archive
  /*	if( !EM_moveSymToExtMem ( SYMSTR(Levelsetfile), HS_NULL ) ){
                  goto failure;//while(1);
          }*/
  //		while(1);

  /*
  1. Find required file size
  2. Check if a memory block of that size is available in archive mem
      If no: Run a garbage collection, go back to 2.

  3. If step 2 has been performed 2 times unsuccessfully, it is not possible to
  archive. If step 2 has been performed successfully, the required archive is
  available. Unarchive the file, write to it and archive it
  */

  /*Autoarchive stuff
          short Tries = 0;

          while(Tries<2){//0 or 1 try

                  if( EM_findEmptySlot() ){//large enough empty slot in archive
     mem?
                          //archive
                          EM_moveSymToExtMem ( SYMSTR(Levelsetfile), HS_NULL );
                          break;//break out of while
                  }
                  else{//No free flash mem blocks that are large enough
                          //Garbage collect, no dialog
                          EM_GC (FALSE);
                          Tries++;
                  }

          }
  */

  return;

failure: // save failed

  Levelsetdata.Savegames[(short)Slot] = 0;

  GrayClearScreen2B(dBufHPL_G, dBufHPD_G);

  // DrawGrayStrExt2B(20+screen_offset_sg_x,40+screen_offset_sg_y,"Saving
  // failed!",A_REPLACE|A_SHADOWED,F_8x10,dBufHPL_G,dBufHPD_G);
  DrawString(20 + screen_offset_sg_x, 40 + screen_offset_sg_y, "Saving failed!",
             A_REPLACE | A_SHADOWED, F_8x10);

  GrayDBufToggleSync_SetPointers();

  /*while(_rowread(0));//wait until key released
  while(!_rowread(0));//wait until key pressed*/
  WaitKeyPress();
};

void Loadgame(char Slot /* , char* Raw0*/) {
  short C;
  short Size;
  //	SYM_ENTRY *Levelsetfile_sym;
  HANDLE Temp;
  //	Raw0 += sizeof(levelsetdata)+21 + 9*(Levelsetdata.Nr_of_files+1);
  // char* Raw = Raw0 + sizeof(levelsetdata)+21 +
  // 9*(Levelsetdata.Nr_of_files+1);
  //	char* Raw = Raw0;
  char *Raw, *Raw0;
  // Raw += Raw%2;
  /*if(((unsigned char)Raw)%2)//temp
          Raw++;*/

  //	Temp = Levelsetfile_get_pointer_and_lock();
  SYM_ENTRY *Levelfile_sym;
  Temp = File_get_pointer_and_lock(Levelsetfile /*,Levelfile_sym*/);

  Raw = Raw0 = HeapDeref(Temp) + 2;
  // copy file content...

  Raw += Levelsetdata.Savegames[(short)Slot];

  Raw += sizeof(saveplayer);

  Levelsetdata.CurrentWorld = *(Raw + 9);

  strcpy(Levelfilename, Raw); // load levelfilename

  Raw += 10;

  // load cardgame
  for (C = 0; C < 18; C++) {
    *(CardGame + C) = *(Raw + C);
  }

  Raw += 18;

  // New: V 1.01 Added check whether or not warp zone is to be loaded. Bugfix
  Load_map((Levelsetdata.CurrentWorld == Levelsetdata.Commonfile
                ? Commonfilename
                : Levelfilename) /*Levelfilename*/); // load map
  // End of new

  // Error: map not found

  short MapSize = Map_data.Height * Map_data.Width +
                  sizeof(map_trigger) * Map_data.Nr_of_triggers +
                  sizeof(map_object) * Map_data.Nr_of_objects;

  // memset(Fg_plane.p.big_vscreen,0x0000,MapSize);

  RLEDecompress(Fg_plane.p.big_vscreen, Raw, MapSize);

  for (C = 0; C < MapSize; C++) { // add deviation
    *(Map + C) += *(Fg_plane.p.big_vscreen + C);
  }

  Raw = Raw0;

  Raw += ((levelsetdata *)Raw)->Savegames[(short)Slot];
  // load player

  memcpy(&SavePlayer, Raw, sizeof(saveplayer));

  if (SavePlayer.Spritebase >= 2) { // large,fire or racoon
    Player.Height = Player.Height2 = 27;
  } else { // small
    Player.Height = Player.Height2 = 16;
  }

  // New: V 1.01 Fixed bug where mario is not immortal after loading a game,
  // even though he used a star just before saving
  if (SavePlayer.Attribs & 0b10000000) {
    Player.Immortal = star_immortal_time;
  };
  // End of new

  Map_data.Border_left = SavePlayer.MapBorderLeft;
  Map_data.Border_right = SavePlayer.MapBorderRight;
  Map_data.Color = SavePlayer.MapColor;

  for (C = 0; C < Map_data.Nr_of_objects; C++) {

    //		(map_object*)Map_objects[C].Handler =
    //(Map_objects[C].Mode==30?Handle_boat:Handle_ship);

    void (*Handler)(void *Map_object);

    if (Map_objects[C].Mode == 30) {
      /*Map_objects[C].*/ Handler = (void (*)(void *))Handle_boat;
    } else {
      if ((Map_objects[C].Mode >= 2) && (Map_objects[C].Mode <= 8)) {
        /*Map_objects[C].*/ Handler = (void (*)(void *))Handle_map_monster;
      } else {
        /*Map_objects[C].*/ Handler = (void (*)(void *))Handle_ship;
      }
    }

    Map_objects[C].Handler = Handler;

    // Handle_map_monster
  }

  //	HeapUnlock(	Levelsetfile_sym->handle );
  HeapUnlock(Temp);
};

HANDLE
File_get_pointer_and_lock(const char *FileName /*,SYM_ENTRY* File_sym*/) {

  SYM_ENTRY *File_sym;
  HANDLE Temp;

  if (!(File_sym = SymFindPtr(SYMSTR(FileName),
                              0))) // flags = 4 will change when the
                                   // documentation of VAT.h is updated
    return H_NULL;                 // error "failed to open levelset"

  if (!(Temp = File_sym->handle))
    return H_NULL; // error "failed to open levelset"

  if (!(HeapLock(File_sym->handle))) // lock memblocks used by file
    return H_NULL;

  /*	if(HeapGetLock (File_sym->handle)==0){
                  while(1);
          }*/

  return Temp;
};
