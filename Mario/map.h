
#ifndef __map__
#define __map__

#include <stddef.h>	//offsetof, for the layout assertions below

//map macros
/*
//Old
#define levels_low						40
#define levels_high						59

#define walkable_low    		   0
#define walkable_high  		    39

#define walkable_node_low 	  15
#define walkable_node_high    39
*/
//New
#define levels_low						45
#define levels_high						64

#define walkable_low    		   0
#define walkable_high  		    44

#define walkable_node_low 	  17
#define walkable_node_high    44


extern char Move_map_objects;
extern unsigned char Treasure;


typedef struct{
	short Width;
	short Height;
	
	short PlayerX;
	short PlayerY;
	
	short WarpX;
	short WarpY;
	
	short FgX;
	short FgY;
	
	short Border_left;
	short Border_right;
		
	short Nr_of_objects;
	short Nr_of_triggers;
	
	short Color;//Determines which tileset to use: Light ground, dark ground, white ground(sky)
	//0: Bright ground (grass/desert)
	//1: White ground (Sky)
	//2: Dark world
	
	short Spare;//For future purposes
		
}map_data;

//Load_map() casts this straight onto the map file's bytes, so its layout is
//the file format and has to stay the 68000's: that ABI aligns a long to 2, so
//nothing here is padded and an entry is 22 bytes. wasm32 aligns a pointer to
//4, which would put two bytes of padding after Treasure and round the struct
//up to 24 - every field past Treasure read from the wrong bytes, drifting a
//further 2 per object. This is the only structure read from a file that has
//pointers in it, and so the only one where the two ABIs disagree.
typedef struct{
	short X;
	short Y;
	
	char Mode;
	char Data0;
	char Data1;
	char MoveX;
	char MoveY;
	
	unsigned char Treasure;
	
	void (*Handler)(void *Map_object);
	
	unsigned short* Sprite;
	unsigned short* Mask;
	
}__attribute__((packed)) map_object;

_Static_assert(offsetof(map_object, Handler) == 10, "map_object must match the file's 68K layout");
_Static_assert(offsetof(map_object, Sprite)  == 14, "map_object must match the file's 68K layout");
_Static_assert(offsetof(map_object, Mask)    == 18, "map_object must match the file's 68K layout");
_Static_assert(sizeof(map_object) == 22, "map_object must match the file's 68K layout");

typedef struct {
	short X;	//XY-pos of the trigger
	short Y;
	short NewX;	//player's new pos
	short NewY;
	short LX;  //Level start position: Used in bidirectional "passages" etc.
	short LY;
	short Border_left;
	short Border_right;
	short LevelNr;
	short Exit;
	//short WorldNr;

	short NewMap;
	short NewBorder_left;
	short NewBorder_right;
	short Color;//Determines which tileset to use: Light ground, dark ground, white ground(sky)
	//0: Bright ground (grass/desert)
	//1: White ground (Sky)
	//2: Dark world
	
}map_trigger;

_Static_assert(sizeof(map_trigger) == 28, "map_trigger must match the file's layout");

/*
//Old
enum Maptiles{M_tile=17,demolished_castle=20,big_castle_visited=21,dock=25,demolished_castle_dark=30,pipe=31,mushrom_house=33,game_house=34,
							small_castle=46,big_castle=32,small_castle_2=50,bowser_castle=59,ground=60,rock=61,locked_door=73,water_map=82,
							locked_door_2=92,
};*/
//New
enum Maptiles{M_tile=19,demolished_castle=22,big_castle_visited=23,dock=27,demolished_castle_dark=32,pipe=33,mushrom_house=35,game_house=36,
							small_castle=51,big_castle=34,small_castle_2=55,bowser_castle=64,ground=65,rock=66,locked_door=78,water_map=87,
							locked_door_2=97,
};

extern map_data Map_data;
extern map_trigger* Map_triggers;
extern map_object* Map_objects;

unsigned char Get_map_tile(short X,short Y);

void Put_map_tile(short X,short Y,unsigned char Tile);


inline void Handle_player_map();

inline void Handle_map_objects();

void Handle_boat(map_object* Ship);

void Handle_ship(map_object* Ship);

void Handle_map_monster(map_object* Monster);

void Enter_enemy_ship(map_object *Ship);

void Enter_mushrom_house(unsigned char Treasure);

void Fight_monster(map_object* Monster);


void Enter_map_anim();

void Enter_level_anim();


short Free_left_map(short X, short Y);

short Free_right_map(short X, short Y);

short Free_up_map(short X, short Y);

short Free_down_map(short X, short Y);



void Replace_map_tile(unsigned char Tile);

void Replace_by_road(short X,short Y);

/*
void Add_money_ship();

void Add_card_game();
*/
void Add_map_event(unsigned char Event);

#endif