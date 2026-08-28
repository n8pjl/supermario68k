#pragma once

// old
/*
#define solid_interactive_low
91 #define solid_interactive_high
160

#define solid_interactive_left_low					91
#define solid_interactive_left_high					100

#define solid_interactive_left_right_low		101
#define solid_interactive_left_right_high		110

#define solid_interactive_right_low					111
#define solid_interactive_right_high				120

#define solid_interactive_up_low 121 #define solid_interactive_up_high
140

#define solid_interactive_up_down_low				141
#define solid_interactive_up_down_high			150

#define solid_interactive_down_low					151
#define solid_interactive_down_high					160


#define non_solid high 89 #define solid_down_low
81 #define solid_down_high
90 #define solid_low
91


#define non_solid_hostile_low					71
#define non_solid_hostile_high				75

#define non_solid_interactive_low					76
#define non_solid_interactive_high				80


#define non_solid_bonus_low						31
#define non_solid_bonus_high					40


#define slope_under_left_low					211
#define slope_under_left_high					220


#define slope_under_right_low					221
#define slope_under_right_high				230


#define slope_over_left_low						231
#define slope_over_left_high					240

#define slope_over_right_low					241
#define slope_over_right_high					250
*/
// end of old

// level macros
/*
//Old too
#define solid_interactive_low
104 #define solid_interactive_high
160

#define solid_down_interactive_low					101
#define solid_down_interactive_high					103

#define solid_interactive_left_low					104
#define solid_interactive_left_high					106

#define solid_interactive_left_right_low		107
#define solid_interactive_left_right_high		109

#define solid_interactive_right_low					110
#define solid_interactive_right_high				112

#define solid_interactive_up_low 113 #define solid_interactive_up_high
140

#define solid_interactive_bounch_low				115
#define solid_interactive_bounch_high				140

#define solid_interactive_up_down_low				141
#define solid_interactive_up_down_high			150

#define solid_interactive_down_low					151
#define solid_interactive_down_high					160



#define non_solid high 103 #define solid_down_low
95 #define solid_down_high
103 #define solid_low
104

#define solid_high
235


#define non_solid_hostile_low					66
#define non_solid_hostile_high				74

#define non_solid_interactive_low					75
#define non_solid_interactive_high				94


#define non_solid_bonus_low						51
#define non_solid_bonus_high					65


#define slope_under_left_low					236
#define slope_under_left_high					241
//helper
#define slope_under_left
slope_under_left_low

#define slope_under_right_low					242
#define slope_under_right_high				247
//helper
#define slope_under_right
slope_under_right_low

#define slope_over_left_low						248
#define slope_over_left_high					251

#define slope_over_right_low					252
#define slope_over_right_high					255
//End of old too
*/

#define solid_interactive_low 113
#define solid_interactive_high 159

#define solid_down_interactive_low 107
#define solid_down_interactive_high 112

#define solid_interactive_left_low 113
#define solid_interactive_left_high 113

#define solid_interactive_left_right_low 114
#define solid_interactive_left_right_high 116

#define solid_interactive_right_low 117
#define solid_interactive_right_high 117

#define solid_interactive_up_low 118
#define solid_interactive_up_high 145

// To be fixed...
#define solid_interactive_bounch_low 120
#define solid_interactive_bounch_high 145

#define solid_interactive_up_down_low 146
#define solid_interactive_up_down_high 148

#define solid_interactive_down_low 149
#define solid_interactive_down_high 159

#define non_solid high 112
#define solid_down_low 101
#define solid_down_high 112
#define solid_low 113

#define solid_high 235

#define non_solid_hostile_low 73
#define non_solid_hostile_high 79

#define non_solid_interactive_low 80
#define non_solid_interactive_high 100

#define non_solid_bonus_low 53
#define non_solid_bonus_high 72

#define slope_under_left_low 236
#define slope_under_left_high 242
// helper
#define slope_under_left slope_under_left_low

#define slope_under_right_low 243
#define slope_under_right_high 249
// helper
#define slope_under_right slope_under_right_low

#define slope_over_left_low 250
#define slope_over_left_high 252

#define slope_over_right_low 253
#define slope_over_right_high 255

// old
/*
enum Tiles {blank=0,pow_item_after=4,dark_gray=13,water_waves_surface=40,
                        coin=51,flower_power=52,
                        treasure_rand=53,treasure_star=54,treasure_whistle=55,treasure_hammer=56,treasure_pwing=57,treasure_cloud=61,treasure_anchor=62,
                        flower_water=58,ladder=77,water_surface=78,water=79,
                        brick=121,bonusbox_1_coin=129,bonusbox_8_coins=136,note=141,note_dark=142,falling_block=151,pow_item=155,bonusbox_after=161,
                        blank_solid=177,wood_block =
180,dark_gray_solid=226,bounching_brick=228};

*/

enum Tiles {
  blank = 0,
  pow_item_after = 4,
  dark_gray = 13,
  water_waves_surface = 40,
  coin = 53,
  flower_power = 54,
  treasure_rand = 55,
  treasure_star = 56,
  treasure_whistle = 57,
  treasure_hammer = 58,
  treasure_pwing = 59,
  treasure_cloud = 63,
  treasure_anchor = 64,
  flower_water = 60,
  ladder = 82,
  water_surface = 83,
  water = 84,
  brick = 126,
  bonusbox_1_coin = 134,
  bonusbox_8_coins = 141,
  note = 146,
  note_dark = 147,
  falling_block = 149,
  pow_item = 153,
  bonusbox_after = 160,
  blank_solid = 176,
  wood_block = 179,
  dark_gray_solid = 225,
  bounching_brick = 227
};

struct leveldata {
  short Width;
  short Height;
  //	short Bg_width;
  short Bg_height;
  short Border_left; // to "hide" secret areas beyond the rightmost part of the
                     // level.
  short Border_right;

  short PlayerX;
  short PlayerY;

  short Boss;

  short Nr_of_enemies;
  short Nr_of_triggers;

  short Nr_of_flying_platforms;

  short Bg_offset;

  char Bg_scrollrate;
  char Background;
  unsigned short Size;
  unsigned short TCSize;

  unsigned char Event; // describes which event to possibly occur when the level
                       // is completed 0: none 1: Card game 5: money ship >50:
                       // Hidden mushrom house
  unsigned char
      Condition;    // Describes under which conditions the event will occur
  unsigned char EX; // EX and EY gives the map position of the event
  unsigned char EY;
  // Special case: When ending level (End of a world): Event indicates which
  // item mario wins when defeating boss.
  // Condition != 0 : Ship Condition == 0 :
  // Castle

  short Spare1; // For future purposes
  short Spare2; // For future purposes
};

struct levelfiledata {
  short Nr_of_levels;
  // TIGCC compiles with 16-bit int (-mshort); this field is 2 bytes in the
  // data files, so it must stay 16-bit here or every level offset shifts.
  short Total_size;

  unsigned short Levels[20]; // holds the offset from start of file to start of
                             // the levelstruct

  // short Map;
  // more to come
  char Mode;
  char Name[20]; // name of the levelset
};

struct bg_data {
  short Height;
  short Width;
};

struct bg_filedata {
  char Nr_of_bgs;

  unsigned short Backgrounds[20]; // holds the offset from start of file to
                                  // start of the bg_struct

  unsigned short Size;
};

extern char *Level, *Map;

short Load_level(char *Levelfile, short Level_nr);

short Generate_handler_1_enemy(short Nr, char Model, char Face,
                               unsigned char D0D1);

void SetBg(short Bg_nr /*,HANDLE Temp*/);

short Load_map(char *Levelfile);

void Free(void *Mem);

extern struct leveldata Leveldata;

extern char Levelfilename[9];
extern char Commonfilename[9];
