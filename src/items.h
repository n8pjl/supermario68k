#pragma once

#include <stdint.h>
#define pow_item_time 300
#define star_immortal_time 500

#define nr_of_items 4

#define nr_of_dummy_platforms 8

#define pu_mushrom_speed 2
#define one_up_mushrom_speed 2
#define star_speed 2

#define elastic_tile_anim_time 5

#define score_100 0
#define score_200 12 * 2
#define score_1000 24 * 2
#define score_1up 36 * 2

struct item {
	int16_t X; // Xpos
	int16_t Y; // Ypos
	int16_t Height;
	int16_t Height2;
	/*	char Data0;
          char Data1;
          char Active;		//if positioned in the active area (inside big
     Vscreen) and "alive", in use
  */
	int16_t Data0;
	int16_t Data1;
	int16_t Active; // if positioned in the active area (inside big Vscreen) and
	// "alive", in use

	void (*Collect)(struct item *Item);
	void (*Handler)(struct item *Item);
	uint16_t *Sprite;
	//	unsigned short* Mask;
	// void *Next;//removed. using array instead
};
struct trigger {
	uint8_t X; // XY-pos of the trigger
	uint8_t Y;
	uint8_t NewX; // player's new pos
	uint8_t NewY;
	uint8_t Border_left;
	uint8_t Border_right;
	uint8_t Anim; // 0: no animation
	// 1: coming up from pipe
	// 2: comming down from pipe
	// 3: coming out of pipe from the left
	// 4: coming out of pipe from the right
	// 5: flying up
	char NewBg;
	char NewBgOffset;
};

// extern item Items[nr_of_items];
extern struct item *Items;

extern struct trigger *Triggers;

extern uint16_t *Itemsprites;
// extern unsigned short *Itemmasks;

void Handleitems();

void Player_collect_bonus(int16_t X, int16_t Y);

void Player_get_coin(int16_t X, int16_t Y);

void Player_collect_coin_water(int16_t X, int16_t Y);

void Player_collect_coin(int16_t X, int16_t Y);

void Player_collect_flower(int16_t X, int16_t Y);

void Player_collect_pu_mushrom(struct item *Item);

void Player_collect_one_up_mushrom(struct item *Item);

void Player_collect_star(struct item *Item);

void Player_collect_leaf(struct item *Item);

void Player_collect_dead_boss(struct item *Item);

void Trigger_pow_item(int16_t X, int16_t Y);

void Convert_brick_coin();

/*char*/ int16_t Free_down(int16_t X, int16_t Y);
/*char*/ int16_t Free_up(int16_t X, int16_t Y);
/*char*/ int16_t Free_right(int16_t X, int16_t *Y, int16_t Height, char Face);
/*char*/ int16_t Free_left(int16_t X, int16_t *Y, int16_t Height, char Face);

void Pipe_down_left_handler(int16_t X, int16_t Y);
void Pipe_down_right_handler(int16_t X, int16_t Y);
void Pipe_down_handler(int16_t X, int16_t Y);

void Pipe_up_left_handler(int16_t X, int16_t Y);
void Pipe_up_right_handler(int16_t X, int16_t Y);
void Pipe_up_handler(int16_t X, int16_t Y);

void Pipe_right_handler(int16_t X, int16_t Y);

void Pipe_left_handler(int16_t X, int16_t Y);

void Falling_block_handler(int16_t X, int16_t Y);

void Note_handler(int16_t X, int16_t Y);

void Pu_2_note_handler(int16_t X, int16_t Y);

void Dark_note_handler(int16_t X, int16_t Y);

void Add_break_brick_anim(int16_t X, int16_t Y);

void Break_brick(int16_t X, int16_t Y);

void Brick_hidden_coins(int16_t X, int16_t Y);

void Bonus_box_coin(int16_t X, int16_t Y);

void Bonus_box_coins(int16_t X, int16_t Y);

void Bonus_box_powerup_1(int16_t X, int16_t Y);

void Bonus_box_powerup_2(int16_t X, int16_t Y);

void Bonus_box_1up(int16_t X, int16_t Y);

void Bonus_box_pow(int16_t X, int16_t Y);

void Bonus_box_star(int16_t X, int16_t Y);

void Bonus_box_climbing_flower(int16_t X, int16_t Y);

void Wood_block_pu2_handler(int16_t X, int16_t Y);

void Door_handler(int16_t X, int16_t Y);

void Hidden_bonusbox_1_coin_handler(int16_t X, int16_t Y);

void Hidden_bonusbox_8_coins_handler(int16_t X, int16_t Y);

void Hidden_bonusbox_1_up_handler(int16_t X, int16_t Y);

void Hidden_bonusbox_pu_1_handler(int16_t X, int16_t Y);

void Hidden_bonusbox_pu_2_handler(int16_t X, int16_t Y);

void Hidden_bonusbox_pow_handler(int16_t X, int16_t Y);

void Hidden_note_handler(int16_t X, int16_t Y);

void Hidden_dark_note_handler(int16_t X, int16_t Y);

void Ladder_handler(int16_t X, int16_t Y);

void Quicksand_handler(int16_t X, int16_t Y);

void Lava_handler(int16_t X, int16_t Y);

void Water_handler(int16_t X, int16_t Y);

void Water_surface_handler(int16_t X, int16_t Y);

void Dark_gray_magic_mode_handler(int16_t X, int16_t Y);

void White_magic_box_handler(int16_t X, int16_t Y);

void Handle_treasure_all(int16_t X, int16_t Y);

void Itemlist_add(char Item);

void Itemlist_remove(char Index);

void Elastic_tile_animate(int16_t X, int16_t Y, uint8_t After, char Attribs,
			  uint8_t Spriteoffset);

void Bonusbox_coin_animate(int16_t X, int16_t Y);

void Mushrom_handler(struct item *Item);

void Leaf_handler(struct item *Item);

void Star_handler(struct item *Item);

void Dead_boss_handler(struct item *Item);

int16_t Find_free_item();

void Add_power_up_1(int16_t X, int16_t Y);
void Add_power_up_2(int16_t X, int16_t Y, int16_t Height);
void Add_pu_mushrom(int16_t X, int16_t Y, int16_t Height);

void Score_anim(int16_t X, int16_t Y, char Socre);

void Execute_trigger(int16_t X, int16_t Y);

void Animate_out_of_wrapper(int16_t T);

void Levelend_handler(int16_t X, int16_t Y);

void Add_dustsky(int16_t X, int16_t Y);
