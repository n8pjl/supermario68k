
#ifndef __map__
#define __map__

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
	
}map_object;

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