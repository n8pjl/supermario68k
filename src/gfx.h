#pragma once

#include <stdint.h>

// #define blank_sprite Tiles
#define blank_sprite (uint16_t *)Fg_plane.p.sprites
// #define black_sprite
// Tiles+3*32
#define black_sprite (uint16_t *)Fg_plane.p.sprites + 3 * 32
//
//
#define monster1_sprite Enemysprites
// #define monster1_mask
// Enemymasks
#define monster1_dead_sprite monster1_sprite + 15 * 3
#define monster1_dead_mask monster1_mask + 15
#define turtle_right_sprite monster1_dead_sprite + 5 * 3
#define turtle_right_mask monster1_dead_mask + 5
#define turtle_left_sprite turtle_right_sprite + 26 * 3
#define turtle_left_mask turtle_right_mask + 16
#define shell_0_sprite turtle_left_sprite + 26 * 3
#define shell_0_mask turtle_left_mask + 16
#define flower1_sprite shell_0_sprite + 16 * 3
#define flower1_mask shell_0_mask + 16
#define flower2_sprite_left_down flower1_sprite + 32 * 6
#define flower2_sprite_left_up flower2_sprite_left_down + 32 * 3
#define flower2_sprite_right_down flower2_sprite_left_up + 32 * 3
#define flower2_sprite_right_up flower2_sprite_right_down + 32 * 3
#define flower2_mask_left_down flower1_mask + 32 * 3
#define flower2_mask_left_up flower2_mask_left_down + 32
#define flower2_mask_right_down flower2_mask_left_up + 32
#define flower2_mask_right_up flower2_mask_right_down + 32
#define monster1_wings1_sprite flower2_sprite_right_up + 32 * 3
#define monster1_wings2_sprite monster1_wings1_sprite + 22 * 3
#define monster1_wings1_mask flower2_mask_right_up + 32
#define monster1_wings2_mask monster1_wings1_mask + 22
#define turtle_fly1_left_sprite monster1_wings2_sprite + 19 * 3
#define turtle_fly1_left_mask monster1_wings2_mask + 19
#define turtle_fly2_left_sprite turtle_fly1_left_sprite + 27 * 3
#define turtle_fly2_left_mask turtle_fly1_left_mask + 27
#define turtle_fly1_right_sprite turtle_fly2_left_sprite + 27 * 3
#define turtle_fly1_right_mask turtle_fly2_left_mask + 27
#define turtle_fly2_right_sprite turtle_fly1_right_sprite + 27 * 3
#define turtle_fly2_right_mask turtle_fly1_right_mask + 27
#define boomerang_guy_left_sprite turtle_fly2_right_sprite + 27 * 3
#define boomerang_guy_left_mask turtle_fly2_right_mask + 27
#define boomerang_guy_right_sprite boomerang_guy_left_sprite + 23 * 3
#define boomerang_guy_right_mask boomerang_guy_left_mask + 23
#define fireball_monster_l_spr boomerang_guy_left_sprite
#define fireball_monster_l_msk boomerang_guy_left_mask
#define fireball_monster_r_spr boomerang_guy_right_sprite
#define fireball_monster_r_msk boomerang_guy_right_mask
#define lavaball_step_1_sprite boomerang_guy_right_sprite + 3 * 23
#define lavaball_mask boomerang_guy_right_mask + 23
#define lavaball_step_2_sprite lavaball_step_1_sprite + 3 * 18
#define circulating_fireball_sprite lavaball_step_2_sprite + 3 * 18
#define circulating_fireball_mask lavaball_mask + 18
#define falling_brick_sprite circulating_fireball_sprite + 3 * 16
#define falling_brick_mask circulating_fireball_mask + 16
#define turtle_skeleton_left_sprite falling_brick_sprite + 19 * 3
#define turtle_skeleton_left_mask falling_brick_mask + 19
#define turtle_skeleton_right_sprite turtle_skeleton_left_sprite + 27 * 3
#define turtle_skeleton_right_mask turtle_skeleton_left_mask + 27
#define crushed_turtle_skeleton_sprite turtle_skeleton_right_sprite + 27 * 3
#define crushed_turtle_skeleton_mask turtle_skeleton_right_mask + 27
#define beetle_left_sprite crushed_turtle_skeleton_sprite + 12 * 3
#define beetle_left_mask crushed_turtle_skeleton_mask + 12
#define beetle_right_sprite beetle_left_sprite + 16 * 3
#define beetle_right_mask beetle_left_mask + 16
#define beetle_shell_sprite beetle_right_sprite + 16 * 3
#define beetle_shell_mask beetle_right_mask + 16
#define jumping_brick_spr beetle_shell_sprite + 16 * 3
#define jumping_brick_msk beetle_shell_mask + 16
#define ghost_waiting_left_spr jumping_brick_spr + 21 * 3
#define ghost_waiting_left_msk jumping_brick_msk + 21
#define ghost_waiting_right_spr ghost_waiting_left_spr + 16 * 3
#define ghost_waiting_right_msk ghost_waiting_left_msk + 16
#define ghost_chasing_left_spr ghost_waiting_right_spr + 16 * 3
#define ghost_chasing_left_msk ghost_waiting_right_msk + 16
#define ghost_chasing_right_spr ghost_chasing_left_spr + 16 * 3
#define ghost_chasing_right_msk ghost_chasing_left_msk + 16
#define fish1_left_spr ghost_chasing_right_spr + 16 * 3
#define fish1_left_msk ghost_chasing_right_msk + 16
#define fish1_right_spr fish1_left_spr + 16 * 3
#define fish1_right_msk fish1_left_msk + 16
#define hedgehog_left_spr fish1_right_spr + 16 * 3
#define hedgehog_left_msk fish1_right_msk + 16
#define hedgehog_right_spr hedgehog_left_spr + 15 * 3
#define hedgehog_right_msk hedgehog_left_msk + 15
#define mad_flower_c_left_spr hedgehog_right_spr + 15 * 3
#define mad_flower_c_left_msk hedgehog_right_msk + 15
#define mad_flower_c_right_spr mad_flower_c_left_spr + 15 * 3
#define mad_flower_c_right_msk mad_flower_c_left_msk + 15
#define mad_flower_o_left_spr mad_flower_c_right_spr + 15 * 3
#define mad_flower_o_left_msk mad_flower_c_right_msk + 15
#define mad_flower_o_right_spr mad_flower_o_left_spr + 15 * 3
#define mad_flower_o_right_msk mad_flower_o_left_msk + 15
#define flying_fish_left_spr mad_flower_o_right_spr + 15 * 3
#define flying_fish_left_msk mad_flower_o_right_msk + 15
#define flying_fish_right_spr flying_fish_left_spr + 16 * 3
#define flying_fish_right_msk flying_fish_left_msk + 16
#define jellyfish_spr flying_fish_right_spr + 16 * 3
#define jellyfish_msk flying_fish_right_msk + 16
#define bomb_walk_left_spr jellyfish_spr + 13 * 3
#define bomb_walk_left_msk jellyfish_msk + 13
#define bomb_walk_right_spr bomb_walk_left_spr + 16 * 3
#define bomb_walk_right_msk bomb_walk_left_msk + 16
#define bomb_deto_left_spr bomb_walk_right_spr + 16 * 3
#define bomb_deto_left_msk bomb_walk_right_msk + 16
#define bomb_deto_right_spr bomb_deto_left_spr + 14 * 3
#define bomb_deto_right_msk bomb_deto_left_msk + 14
#define bomb_star_spr bomb_deto_right_spr + 14 * 3
#define bomb_star_msk bomb_deto_right_msk + 14
#define bottom_flower_l1 bomb_star_spr + 8 * 3
#define bottom_flower_r1 bottom_flower_l1 + 28 * 3
#define bottom_flower_l2 bottom_flower_r1 + 28 * 3
#define bottom_flower_r2 bottom_flower_l2 + 30 * 3
#define bottom_flower_l3 bottom_flower_r2 + 30 * 3
#define bottom_flower_r3 bottom_flower_l3 + 32 * 3

#define brick_fragment_sprite Smallsprites

#define one_spr brick_fragment_sprite + 16 + 8
#define two_spr one_spr + 30
#define three_spr two_spr + 30
#define five_spr three_spr + 30

#define triangle_e_spr five_spr + 30
#define triangle_f_spr triangle_e_spr + 16
#define triangle_msk triangle_f_spr + 16
// Smallmasks
// triangle_f_spr + 16
// brick_fragment_mask+8

// #define fireball_hit_anim					Masks

#define pu_mushrom_sprite Itemsprites
#define mushrom_mask 0 // Itemmasks
#define one_up_mushrom_sprite pu_mushrom_sprite + 16 * 3
#define star_anim one_up_mushrom_sprite + 16 * 3
#define star_mask mushrom_mask + 16
#define leaf_l_sprite star_anim + 16 * 3
#define leaf_l_mask star_mask + 16
#define leaf_r_sprite leaf_l_sprite + 14 * 3
#define leaf_r_mask leaf_l_mask + 14
#define flower_spr leaf_r_sprite + 14 * 3
#define flower_msk leaf_r_mask + 14
#define hammer_i_spr flower_spr + 16 * 3
#define hammer_i_msk flower_msk + 16
#define whistle_spr hammer_i_spr + 16 * 3
#define whistle_msk hammer_i_msk + 16
#define p_wing_spr whistle_spr + 16 * 3
#define p_wing_msk whistle_msk + 16
#define magic_wand_spr p_wing_spr + 16 * 3
#define magic_wand_msk p_wing_msk + 16
#define cloud_item_spr magic_wand_spr + 16 * 3
#define cloud_item_msk cloud_item_spr + 16 * 2
#define anchor_spr cloud_item_spr + 16 * 3
#define anchor_msk cloud_item_msk + 16

// #define bb_coin_anim_sprite				brick_tile_sprite+32
// #define bb_coin_anim_mask bbox_note_mask+16
#define bb_coin_anim_sprite Sprites
#define bb_coin_anim_mask Masks + 48

#define score_sprites bb_coin_anim_sprite + 300
#define score_masks bb_coin_anim_mask + 100

#define score_sprite_100 score_sprites
#define score_sprite_200 score_sprite_100 + 16 + 8
#define score_sprite_1000 score_sprite_200 + 16 + 8
#define score_sprite_1up score_sprite_1000 + 16 + 8

#define fireball_anim_16_spr score_sprites + 16 * 4 + 8 * 4

#define boomerang_anim_16_spr fireball_anim_16_spr + 8 * 2 * 4 + 8 * 4

#define up_sprite boomerang_anim_16_spr + 48 * 4

#define cannonball_horiz_left_spr up_sprite + 15 * 2
#define cannonball_horiz_right_spr cannonball_horiz_left_spr + 14 * 2 + 14

#define cannonball_spr cannonball_horiz_right_spr + 14 * 2 + 14

#define ship_spr cannonball_spr + 12 * 3

#define ship_msk fireball_hit_anim + 16 * 3

#define map_monster_left_spr ship_spr + 16 * 3
#define map_monster_left_msk ship_msk + 16

#define map_monster_right_spr map_monster_left_spr + 16 * 3
#define map_monster_right_msk map_monster_left_msk + 16

#define p_bright_spr map_monster_right_spr + 16 * 3
#define p_dark_spr p_bright_spr + 16
#define p_msk p_dark_spr + 16

#define boss_shot_spr p_msk + 8

#define map_boat_spr boss_shot_spr + 12 * 3
#define map_boat_msk p_msk + 8

#define card_top map_boat_spr + 16 * 3

#define hammer_anim_spr card_top + 22 * 2
#define hammer_anim_msk map_boat_msk + 16

#define underwater_shot hammer_anim_spr + 48 * 4

#define game_mushrom_l underwater_shot + 8 * 3
#define game_mushrom_r game_mushrom_l + 38 * 2
#define game_star_l game_mushrom_r + 38 * 2
#define game_star_r game_star_l + 38 * 2
#define game_flower_l game_star_r + 38 * 2
#define game_flower_r game_flower_l + 38 * 2

#define titlescreen_gfx game_flower_r + 38 * 2

#define fireball_hit_anim titlescreen_gfx + 654

#define money_ship_spr fireball_hit_anim + 16 * 3

#define card_game_spr money_ship_spr + 16 * 3

#define mhouse_hidden_spr card_game_spr + 16 * 3

#define splash_gfx mhouse_hidden_spr + 16 * 3

#define princess_spr splash_gfx + 86

#define big_fireball_left_spr princess_spr + 30 * 3

#define big_fireball_right_spr big_fireball_left_spr + 10 * 3

#define falling_block_spr big_fireball_right_spr + 10 * 3

#define warp_cloud_spr falling_block_spr + 16 * 3

#define racoon_tale_spr Mariosprites + 740 * 2 + 16 * 2 * 4 + 27 * 2 * 8

#define racoon_tale_msk Mariomasks + 434 + 16 * 4 + 27 * 4

#define racoon_cap_spr racoon_tale_spr + 9 * 2 * 10
#define racoon_cap_msk racoon_tale_msk + 9 * 10

// h=23
#define boss1_spikes_spr Boss_sprites
// h=27
#define boss1_run1_spr boss1_spikes_spr + 23 * 2 * 2 + 23 * 2
// h=27
#define boss1_run2_spr boss1_run1_spr + 27 * 2 * 2 + 27 * 2
// h=17
#define boss1_hurt_spr boss1_run2_spr + 27 * 2 * 2 + 27 * 2
// h=16
#define boss1_killed_spr boss1_hurt_spr + 17 * 2 * 2 + 17 * 2
#define boss1_killed_msk boss1_killed_spr + 16 * 2

#define boss2_left_spr boss1_killed_spr + 16 * 2 + 16

#define boss2_right_spr boss2_left_spr + 30 * 2 * 2 + 30 * 2

#define boss2_hurt_spr boss2_right_spr + 30 * 2 * 2 + 30 * 2

#define bowser_facing_spr boss2_hurt_spr + 17 * 3 * 2

#define bowser_run_left_spr bowser_facing_spr + 38 * 6

#define bowser_run_right_spr bowser_run_left_spr + 38 * 6

#define bowser_angry bowser_run_right_spr + 38 * 3 * 2

/*
typedef struct{//Optimize! Skip some unnesseccarry variables, use macros in
calculations instead! unsigned short Nr_of_fg_tiles; unsigned short
Nr_of_tilemasks; unsigned short Nr_of_fg_animations; unsigned short
Nr_of_fg_mask_animations; unsigned short Nr_of_bg_tiles; unsigned short
Nr_of_map_tiles; unsigned short Nr_of_map_animations;

        unsigned short Total_size;		//IN BYTES!!!
}tilefiledata;
*/
#define Nr_of_fg_tiles 206
#define Nr_of_tilemasks 81
#define Nr_of_fg_animations 256
#define Nr_of_fg_mask_animations 256
#define Nr_of_bg_tiles 127
#define Nr_of_map_tiles 151
#define Nr_of_map_animations 110
// 105

/*
typedef struct{//Optimize! Skip some unnesseccarry variables, use macros in
calculations instead! unsigned short Size_of_mariosprites;	//nr of shorts.
this is valid for the next variables too... unsigned short Size_of_mariomasks;
        unsigned short Size_of_marioanimtab;
        unsigned short Size_of_enemysprites;
//	unsigned short Size_of_enemymasks;
        unsigned short Size_of_smallsprites;
//	unsigned short Size_of_smallmasks;
        unsigned short Size_of_sprites;
//	unsigned short Size_of_masks;
        unsigned short Size_of_itemsprites;
//	unsigned short Size_of_itemmasks;
        unsigned short Size_of_boss_sprites;
//	unsigned short Size_of_boss_masks;
        unsigned short Total_size;		//IN BYTES!!! (that means NOT
short!) if size exseeds 64k (not likely) then use multiple files!
}spritefiledata;
*/

#define Size_of_mariosprites 2320
#define Size_of_mariomasks 746
#define Size_of_marioanimtab 88
#define Size_of_enemysprites 3690
#define Size_of_smallsprites 184
#define Size_of_sprites (2844 + 30 * 3 + 60 + 16 * 3 + 16 * 3)
#define Size_of_itemsprites 564
#define Size_of_boss_sprites (1074 + 228 + 456 + 240)

// The background file, which SetBg() indexes into per level. The tile and
// sprite files are read once, in Load_gfx_from_file(), and reached through the
// pointers below from then on.
extern const uint8_t *Bg_file_data;
/*
extern unsigned short *Tiles;
extern unsigned short *Tilemasks;

extern unsigned short *Bg_tiles;
extern unsigned short *Map_tiles;

extern short *Fg_animations;
extern short *Fg_mask_animations;
extern short *Map_animations;
*/
extern const uint16_t *Mariosprites;
extern const uint16_t *Mariomasks;
extern const int16_t (*Marioanimtab)[11];
extern const uint16_t *Enemysprites;
// extern unsigned short *Enemymasks;
extern const uint8_t *Smallsprites;
// extern unsigned char  *Smallmasks;
extern const uint16_t *Sprites;
// extern unsigned short *Masks;
extern const uint16_t *Boss_sprites;
// extern unsigned short *Boss_masks;
// extern unsigned short *Itemsprites;
// extern unsigned short *Itemmasks;

int16_t Load_gfx_from_file();
