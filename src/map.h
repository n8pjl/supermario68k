#pragma once

#include <stddef.h> //offsetof, for the layout assertions below
#include <stdint.h>

// map macros
/*
//Old
#define levels_low						40
#define levels_high						59

#define walkable_low    		   0
#define walkable_high  		    39

#define walkable_node_low 	  15
#define walkable_node_high    39
*/
// New
#define levels_low 45
#define levels_high 64

#define walkable_low 0
#define walkable_high 44

#define walkable_node_low 17
#define walkable_node_high 44

extern char Move_map_objects;
extern uint8_t Treasure;

struct map_data {
	int16_t Width;
	int16_t Height;

	int16_t PlayerX;
	int16_t PlayerY;

	int16_t WarpX;
	int16_t WarpY;

	int16_t FgX;
	int16_t FgY;

	int16_t Border_left;
	int16_t Border_right;

	int16_t Nr_of_objects;
	int16_t Nr_of_triggers;

	int16_t Color; // Determines which tileset to use: Light ground, dark ground,
	// white ground(sky)
	// 0: Bright ground (grass/desert)
	// 1: White ground (Sky)
	// 2: Dark world

	int16_t Spare; // For future purposes
};

// Load_map() casts this straight onto the map file's bytes, so its layout is
// the file format and has to stay the 68000's: that ABI aligns a long to 2, so
// nothing here is padded and an entry is 22 bytes. wasm32 aligns a pointer to
// 4, which would put two bytes of padding after Treasure and round the struct
// up to 24 - every field past Treasure read from the wrong bytes, drifting a
// further 2 per object. This is the only structure read from a file that has
// pointers in it, and so the only one where the two ABIs disagree.
struct map_object {
	int16_t X;
	int16_t Y;

	char Mode;
	char Data0;
	char Data1;
	char MoveX;
	char MoveY;

	uint8_t Treasure;

	void (*Handler)(void *Map_object);

	uint16_t *Sprite;
	uint16_t *Mask;

} __attribute__((packed));

_Static_assert(offsetof(struct map_object, Handler) == 10,
	       "map_object must match the file's 68K layout");
_Static_assert(offsetof(struct map_object, Sprite) == 14,
	       "map_object must match the file's 68K layout");
_Static_assert(offsetof(struct map_object, Mask) == 18,
	       "map_object must match the file's 68K layout");
_Static_assert(sizeof(struct map_object) == 22,
	       "map_object must match the file's 68K layout");

struct map_trigger {
	int16_t X; // XY-pos of the trigger
	int16_t Y;
	int16_t NewX; // player's new pos
	int16_t NewY;
	int16_t LX; // Level start position: Used in bidirectional "passages" etc.
	int16_t LY;
	int16_t Border_left;
	int16_t Border_right;
	int16_t LevelNr;
	int16_t Exit;
	// short WorldNr;

	int16_t NewMap;
	int16_t NewBorder_left;
	int16_t NewBorder_right;
	int16_t Color; // Determines which tileset to use: Light ground, dark ground,
	// white ground(sky)
	// 0: Bright ground (grass/desert)
	// 1: White ground (Sky)
	// 2: Dark world
};

_Static_assert(sizeof(struct map_trigger) == 28,
	       "map_trigger must match the file's layout");

/*
//Old
enum
Maptiles{M_tile=17,demolished_castle=20,big_castle_visited=21,dock=25,demolished_castle_dark=30,pipe=31,mushrom_house=33,game_house=34,
                                                        small_castle=46,big_castle=32,small_castle_2=50,bowser_castle=59,ground=60,rock=61,locked_door=73,water_map=82,
                                                        locked_door_2=92,
};*/
// New
enum Maptiles {
	M_tile = 19,
	demolished_castle = 22,
	big_castle_visited = 23,
	dock = 27,
	demolished_castle_dark = 32,
	pipe = 33,
	mushrom_house = 35,
	game_house = 36,
	small_castle = 51,
	big_castle = 34,
	small_castle_2 = 55,
	bowser_castle = 64,
	ground = 65,
	rock = 66,
	locked_door = 78,
	water_map = 87,
	locked_door_2 = 97,
};

extern struct map_data Map_data;
extern struct map_trigger *Map_triggers;
extern struct map_object *Map_objects;

uint8_t Get_map_tile(int16_t X, int16_t Y);

void Put_map_tile(int16_t X, int16_t Y, uint8_t Tile);

void Handle_player_map();

void Handle_map_objects();

void Handle_boat(struct map_object *Ship);

void Handle_ship(struct map_object *Ship);

void Handle_map_monster(struct map_object *Monster);

void Enter_enemy_ship(struct map_object *Ship);

void Enter_mushrom_house(uint8_t Treasure);

void Fight_monster(struct map_object *Monster);

void Enter_map_anim();

void Enter_level_anim();

int16_t Free_left_map(int16_t X, int16_t Y);

int16_t Free_right_map(int16_t X, int16_t Y);

int16_t Free_up_map(int16_t X, int16_t Y);

int16_t Free_down_map(int16_t X, int16_t Y);

void Replace_map_tile(uint8_t Tile);

void Replace_by_road(int16_t X, int16_t Y);

/*
void Add_money_ship();

void Add_card_game();
*/
void Add_map_event(uint8_t Event);
