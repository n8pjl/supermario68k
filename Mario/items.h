
#ifndef __items__
#define __items__

#define pow_item_time						300
#define star_immortal_time			500

#define nr_of_items							4

#define nr_of_dummy_platforms   8


#define pu_mushrom_speed				2
#define one_up_mushrom_speed		2
#define star_speed							2

#define elastic_tile_anim_time	5

#define score_100								0
#define score_200								12*2
#define score_1000							24*2	
#define score_1up								36*2

typedef struct item {
	short X;		//Xpos
	short Y;		//Ypos
	short Height;
	short Height2;
/*	char Data0;
	char Data1;
	char Active;		//if positioned in the active area (inside big Vscreen) and "alive", in use
*/
	short Data0;
	short Data1;
	short Active;		//if positioned in the active area (inside big Vscreen) and "alive", in use

	void (*Collect)(struct item *Item);
  void (*Handler)(struct item *Item);
	unsigned short* Sprite;
//	unsigned short* Mask;
	//void *Next;//removed. using array instead
}item;



typedef struct {
	unsigned char X;	//XY-pos of the trigger
	unsigned char Y;
	unsigned char NewX;	//player's new pos
	unsigned char NewY;
	unsigned char Border_left;
	unsigned char Border_right;
	unsigned char Anim;		//0: no animation
								//1: coming up from pipe
								//2: comming down from pipe
								//3: coming out of pipe from the left
								//4: coming out of pipe from the right
								//5: flying up
	char NewBg;
	char NewBgOffset;
}trigger;



//extern item Items[nr_of_items];
extern item* Items;



extern trigger *Triggers;


extern unsigned short *Itemsprites;
//extern unsigned short *Itemmasks;

void Handleitems();

void Player_collect_bonus( short X, short Y);

void Player_get_coin( short X, short Y);

void Player_collect_coin_water( short X, short Y);

void Player_collect_coin( short X, short Y);

void Player_collect_flower( short X, short Y);


void Player_collect_pu_mushrom(item* Item);

void Player_collect_one_up_mushrom(item* Item);

void Player_collect_star(item* Item);

void Player_collect_leaf(item* Item);

void Player_collect_dead_boss(item* Item);



void Trigger_pow_item( short X, short Y);

void Convert_brick_coin();

/*char*/short Free_down(short X,short Y);
/*char*/short Free_up(short X,short Y);
/*char*/short Free_right(short X,short* Y,short Height,char Face);
/*char*/short Free_left(short X,short* Y,short Height,char Face);

void Pipe_down_left_handler( short X, short Y);
void Pipe_down_right_handler( short X, short Y);
void Pipe_down_handler( short X, short Y);

void Pipe_up_left_handler( short X, short Y);
void Pipe_up_right_handler( short X, short Y);
void Pipe_up_handler( short X, short Y);


void Pipe_right_handler( short X, short Y);


void Pipe_left_handler( short X, short Y);

void Falling_block_handler( short X, short Y);

void Note_handler( short X, short Y);

void Pu_2_note_handler( short X, short Y );

void Dark_note_handler( short X, short Y);

void Add_break_brick_anim( short X, short Y);

void Break_brick( short X, short Y);

void Brick_hidden_coins( short X, short Y);

void Bonus_box_coin( short X, short Y);

void Bonus_box_coins( short X, short Y);

void Bonus_box_powerup_1( short X, short Y);

void Bonus_box_powerup_2( short X, short Y);

void Bonus_box_1up( short X, short Y);

void Bonus_box_pow( short X, short Y);

void Bonus_box_star( short X, short Y);

void Bonus_box_climbing_flower( short X, short Y);

void Wood_block_pu2_handler( short X, short Y);

void Door_handler( short X, short Y );

void Hidden_bonusbox_1_coin_handler( short X, short Y );

void Hidden_bonusbox_8_coins_handler( short X, short Y );

void Hidden_bonusbox_1_up_handler( short X, short Y );

void Hidden_bonusbox_pu_1_handler( short X, short Y );

void Hidden_bonusbox_pu_2_handler( short X, short Y );

void Hidden_bonusbox_pow_handler( short X, short Y );

void Hidden_note_handler( short X, short Y );

void Hidden_dark_note_handler( short X, short Y );

void Ladder_handler( short X, short Y );

void Quicksand_handler( short X, short Y );

void Lava_handler( short X, short Y );

void Water_handler( short X, short Y );

void Water_surface_handler( short X, short Y );

void Dark_gray_magic_mode_handler( short X, short Y );

void White_magic_box_handler( short X, short Y );



void Handle_treasure_all( short X, short Y );




void Itemlist_add(char Item); 

void Itemlist_remove(char Index);





void Elastic_tile_animate(short X, short Y, unsigned char After, char Attribs, unsigned char Spriteoffset);

void Bonusbox_coin_animate(short X, short Y);


void Mushrom_handler(item* Item);

void Leaf_handler(item* Item);

void Star_handler(item* Item);

void Dead_boss_handler(item* Item);


short Find_free_item();


void Add_power_up_1(short X, short Y);
void Add_power_up_2(short X, short Y, short Height);
void Add_pu_mushrom(short X, short Y, short Height);

void Score_anim(short X, short Y, char Socre);


void Execute_trigger(short X, short Y);

void Animate_out_of_wrapper(short T);

void Levelend_handler(short X, short Y);

void Add_dustsky(short X, short Y);

#endif