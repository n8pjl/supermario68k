#include "savegame.h"
#include "compat/assets.h"
#include "compat/extgraph.h"
#include "compat/graph.h"
#include "gameloop.h"
#include "items.h"
#include "level.h"
#include "levelset.h"
#include "map.h"
#include "player.h"
#include "render.h"
#include "rle.h"
#include "scankeys.h"
#include "smallgames.h"
#include "stringcopy.h"
#include <stdint.h>
#include <string.h>

// Reports a failed save and marks the slot empty. Its own function because
// C++ will not let the two failure paths jump forward past the initialized
// locals in between, the way the C goto did.
static void Save_failed(char Slot)
{
	Levelsetdata.Savegames[(int16_t)Slot] = 0;

	GrayClearScreen2B(dBufHPL_G, dBufHPD_G);

	// DrawGrayStrExt2B(20+screen_offset_sg_x,40+screen_offset_sg_y,"Saving
	// failed!",A_REPLACE|A_SHADOWED,F_8x10,dBufHPL_G,dBufHPD_G);
	DrawString(20 + screen_offset_sg_x, 40 + screen_offset_sg_y,
		   "Saving failed!", A_REPLACE | A_SHADOWED, F_8x10);

	GrayDBufToggleSync_SetPointers();

	/*while(_rowread(0));//wait until key released
  while(!_rowread(0));//wait until key pressed*/
	WaitKeyPress();
}

void Savegame(char Slot)
{
	int16_t C;
	int16_t Size;

	struct asset *Levelset = Asset_find(Levelsetfile);

	if (!Levelset) {
		Save_failed(Slot);
		return;
	}
	// copy file content...

	// levelsetdata, title and filenames

	Size = sizeof(struct levelsetdata) + 21 +
	       9 * (Levelsetdata.Nr_of_files + 1);

	char *TempBuffer = Fg_plane.p.big_vscreen;
	memcpy(TempBuffer /*Fg_plane.p.big_vscreen*/, Levelset->data, Size);

	// copy 2 of 3 saveslots, excluding the one to be used

	for (C = 0; C < 3; C++) {
		if ((C != Slot) && (Levelsetdata.Savegames[C] != 0)) {
			Size += Size % 2;
			memcpy(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size,
			       Levelset->data + Levelsetdata.Savegames[C],
			       Levelsetdata.Savegame_size[C]);
			// update saveslot pointer
			((struct levelsetdata *)
				 TempBuffer /*Fg_plane.p.big_vscreen*/)
				->Savegames[C] = Levelsetdata.Savegames[C] =
				Size;
			// Levelsetdata.Savegames[C] = Size;

			Size += Levelsetdata.Savegame_size[C];
		}

		// Levelsetdata.Savegames[C];
		// Levelsetdata.Savegame_size[C];
	}

	// generate savegame :
	// save player

	Size += Size % 2; // temp

	// update saveslot pointer
	// Levelsetdata.Savegames[(short)Slot] = Size;
	((struct levelsetdata *)TempBuffer /*Fg_plane.p.big_vscreen*/)
		->Savegames[(int16_t)Slot] =
		Levelsetdata.Savegames[(int16_t)Slot] = Size;

	SavePlayer.MapBorderLeft = Map_data.Border_left;
	SavePlayer.MapBorderRight = Map_data.Border_right;
	SavePlayer.MapColor = Map_data.Color;

	memcpy(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size, &SavePlayer,
	       sizeof(struct saveplayer));

	Size += sizeof(struct saveplayer);

	// Map_data.Height*Map_data.Width+sizeof(map_trigger)*Map_data.Nr_of_triggers+sizeof(map_object)*(Map_data.Nr_of_objects)
	// ))

	StringCopy(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size, Levelfilename);

	*(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size + 9) =
		Levelsetdata.CurrentWorld;

	//	Size += 10;

	// save cardgame
	for (C = 0; C < 18; C++) {
		*(TempBuffer /*Fg_plane.p.big_vscreen*/ + Size + C + 10) =
			*(CardGame + C);
	}
	Size += (18 + 10);

	int16_t MapSize = Map_data.Height * Map_data.Width +
			  sizeof(struct map_trigger) * Map_data.Nr_of_triggers +
			  sizeof(struct map_object) * Map_data.Nr_of_objects;

	char *TempBuffer2 = Fg_mask.p.big_vscreen;
	memcpy(TempBuffer2 /*Fg_mask.p.big_vscreen*/, Map,
	       MapSize); // copy possibly modified map with objects and triggers

	int16_t OldPCX = SavePlayer.PrevCompX;
	int16_t OldPCY = SavePlayer.PrevCompY;
	int16_t OldMapX = SavePlayer.MapX;
	int16_t OldMapY = SavePlayer.MapY;
	int16_t OldL = SavePlayer.L;
	int16_t OldFgX = FgX;
	int16_t OldFgY = FgY;

	// New: V 1.01 Added check whether or not warp zone is to be loaded. Bugfix
	Load_map((Levelsetdata.CurrentWorld == Levelsetdata.Commonfile ?
			  Commonfilename :
			  Levelfilename)); // load original map
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

	int16_t RLESize = RLECompress(
		(uint8_t *)TempBuffer /*Fg_plane.p.big_vscreen*/ + Size,
		(uint8_t *)TempBuffer2 /*Fg_mask.p.big_vscreen*/, MapSize);
	// short SlotSize = sizeof(saveplayer)+10+RLESize;
	for (C = 0; C < MapSize; C++) { // Undo loading of original map
		*(Map + C) += *(TempBuffer2 /*Fg_mask.p.big_vscreen*/ + C);
	}

	Map_data.Border_left = SavePlayer.MapBorderLeft;
	Map_data.Border_right = SavePlayer.MapBorderRight;

	int16_t Slotsize = sizeof(struct saveplayer) + 10 + 18 + RLESize;
	Size += RLESize;

	((struct levelsetdata *)TempBuffer /*Fg_plane.p.big_vscreen*/)
		->Savegame_size[(int16_t)Slot] =
		Levelsetdata.Savegame_size[(int16_t)Slot] = Slotsize;
	// Levelsetdata.Savegame_size[(short)Slot] = Slotsize;
	//((levelsetdata*)Fg_plane.p.big_vscreen)->Savegame_size[(short)Slot] =
	// Size-((levelsetdata*)Fg_plane.p.big_vscreen)->Savegames[(short)Slot];
	//((levelsetdata*)Fg_plane.p.big_vscreen)->Savegame_size[(short)Slot] =
	// Levelsetdata.Savegame_size[(short)Slot] = SlotSize;

	// memcpy(Fg_plane.p.big_vscreen,&Levelsetdata,sizeof(levelsetdata));

	// The whole levelset, saveslots and all, is what gets written back: the
	// buffer above was assembled out of the file it replaces.
	if (!Asset_save(Levelset, TempBuffer, Size)) {
		Save_failed(Slot);
		return;
	}

	/*
  1. Find required file size
  2. Check if a memory block of that size is available in archive mem
      If no: Run a garbage collection, go back to 2.

  3. If step 2 has been performed 2 times unsuccessfully, it is not possible
  to archive. If step 2 has been performed successfully, the required archive
  is available. Unarchive the file, write to it and archive it
  */

	return;
}

void Loadgame(char Slot)
{
	int16_t C;
	//	Raw0 += sizeof(levelsetdata)+21 + 9*(Levelsetdata.Nr_of_files+1);
	// char* Raw = Raw0 + sizeof(levelsetdata)+21 +
	// 9*(Levelsetdata.Nr_of_files+1);
	//	char* Raw = Raw0;
	const char *Raw, *Raw0;
	// Raw += Raw%2;
	/*if(((unsigned char)Raw)%2)//temp
          Raw++;*/

	const struct asset *Levelset = Asset_find(Levelsetfile);

	if (!Levelset) {
		return;
	}

	Raw = Raw0 = (const char *)Levelset->data;
	// copy file content...

	Raw += Levelsetdata.Savegames[(int16_t)Slot];

	Raw += sizeof(struct saveplayer);

	Levelsetdata.CurrentWorld = *(Raw + 9);

	strcpy(Levelfilename, Raw); // load levelfilename

	Raw += 10;

	// load cardgame
	for (C = 0; C < 18; C++) {
		*(CardGame + C) = *(Raw + C);
	}

	Raw += 18;

	// New: V 1.01 Added check whether or not warp zone is to be loaded. Bugfix
	Load_map((Levelsetdata.CurrentWorld == Levelsetdata.Commonfile ?
			  Commonfilename :
			  Levelfilename) /*Levelfilename*/); // load map
	// End of new

	// Error: map not found

	int16_t MapSize = Map_data.Height * Map_data.Width +
			  sizeof(struct map_trigger) * Map_data.Nr_of_triggers +
			  sizeof(struct map_object) * Map_data.Nr_of_objects;

	// memset(Fg_plane.p.big_vscreen,0x0000,MapSize);

	RLEDecompress((uint8_t *)Fg_plane.p.big_vscreen, (const uint8_t *)Raw,
		      MapSize);

	for (C = 0; C < MapSize; C++) { // add deviation
		*(Map + C) += *(Fg_plane.p.big_vscreen + C);
	}

	Raw = Raw0;

	Raw += ((const struct levelsetdata *)Raw)->Savegames[(int16_t)Slot];
	// load player

	memcpy(&SavePlayer, Raw, sizeof(struct saveplayer));

	if (SavePlayer.Spritebase >= 2) { // large,fire or racoon
		Player.Height = Player.Height2 = 27;
	} else { // small
		Player.Height = Player.Height2 = 16;
	}

	// New: V 1.01 Fixed bug where mario is not immortal after loading a game,
	// even though he used a star just before saving
	if (SavePlayer.Attribs & 0b10000000) {
		Player.Immortal = star_immortal_time;
	}
	// End of new

	Map_data.Border_left = SavePlayer.MapBorderLeft;
	Map_data.Border_right = SavePlayer.MapBorderRight;
	Map_data.Color = SavePlayer.MapColor;

	for (C = 0; C < Map_data.Nr_of_objects; C++) {
		//		(map_object*)Map_objects[C].Handler =
		//(Map_objects[C].Mode==30?Handle_boat:Handle_ship);

		void (*Handler)(void *Map_object);

		if (Map_objects[C].Mode == 30) {
			/*Map_objects[C].*/ Handler =
				(void (*)(void *))Handle_boat;
		} else {
			if ((Map_objects[C].Mode >= 2) &&
			    (Map_objects[C].Mode <= 8)) {
				/*Map_objects[C].*/ Handler =
					(void (*)(void *))Handle_map_monster;
			} else {
				/*Map_objects[C].*/ Handler =
					(void (*)(void *))Handle_ship;
			}
		}

		Map_objects[C].Handler = Handler;

		// Handle_map_monster
	}
}
