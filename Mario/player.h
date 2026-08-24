
#ifndef __player__
#define __player__

//mario sprites 
#define small_left_sprites						Mariosprites
#define stand_left_small_sprite				Mariosprites
#define walk_left_small_sprite				stand_left_small_sprite+32
#define run_left_small_sprite					walk_left_small_sprite+32
#define jump_left_small_sprite				run_left_small_sprite+32

#define small_right_sprites						jump_left_small_sprite+32
#define stand_right_small_sprite			jump_left_small_sprite+32
#define walk_right_small_sprite				stand_right_small_sprite+32
#define run_right_small_sprite				walk_right_small_sprite+32
#define jump_right_small_sprite				run_right_small_sprite+32

#define large_left_sprites						jump_right_small_sprite+32
#define stand_left_large_sprite				jump_right_small_sprite+32
#define	walk1_left_large_sprite				stand_left_large_sprite+54
#define	walk2_left_large_sprite				walk1_left_large_sprite+54


#define large_right_sprites						walk2_left_large_sprite+54
#define stand_right_large_sprite			walk2_left_large_sprite+54
#define walk1_right_large_sprite			stand_right_large_sprite+54
#define walk2_right_large_sprite			walk1_right_large_sprite+54


//mario masks
#define small_left_masks							Mariomasks
#define stand_left_small_mask					Mariomasks
#define walk_left_small_mask					stand_left_small_mask+16
#define run_left_small_mask						walk_left_small_mask+16
#define jump_left_small_mask					run_left_small_mask+16

#define small_right_masks							jump_left_small_mask+16
#define stand_right_small_mask				jump_left_small_mask+16
#define walk_right_small_mask					stand_right_small_mask+16
#define run_right_small_mask					walk_right_small_mask+16
#define jump_right_small_mask					run_right_small_mask+16

#define large_left_masks							jump_right_small_mask+16
#define stand_left_large_mask					jump_right_small_mask+16
#define	walk1_left_large_mask					stand_left_large_mask+27
#define	walk2_left_large_mask					walk1_left_large_mask+27
//more

#define large_right_masks							walk2_left_large_mask+27
#define stand_right_large_mask				walk2_left_large_mask+27
#define walk1_right_large_mask				stand_right_large_mask+27
#define walk2_right_large_mask				walk1_right_large_mask+27
//more





#define anim_lenght							1

#define flycondition						16
#define flyspeed								(4-1)
#define flylength								(48+16)

#define jumpheight							56
//48
#define player_fireball_speed		4

#define PLAYER_WALKSPEED				2
#define PLAYER_RUNSPEED 				4

#define nr_of_player_smallshots		2

#define itemlist_length					20



typedef struct {
	short X;		//Xpos
	short Y;		//Ypos
	
	char Xoffset;
	char Yoffset;

	short Immortal;	//used when recently wounded and when player recently got a star. Used as a (frame)counter,counts down to 0.	

//	short Life;
//	short Lives;
//	char Attribs;		//bit7: set when mario is "super",has got a star.
									//bit6 set: Can throw fireballs.
									//bit5 set: has the racoon suit.
									//bit4 set: Draw player before foreground, used in some effects		
									//bit3 set: P-wing: Always able to fly
	char Racoonspin;

	char Face;			//direction.   (+ == right) && (- == left)

	char PrevFace;//fly code
	short SlopeAbove;
	short SlopeDownUnder;
//	char Coins;
	
//	unsigned int Score;//nr of houndred points,get padded with "00"
	
	char Walkspeed;
	char Walkspeed2;		//contents normal speed when run
	short Jumpspeed;
	short Fallspeed;
	
	short Max_jumpheight;
	
	
	short IsJumping;
	short IsFalling;
	char IsInWater;	//will possibly be replaced by something using Attribs. 

	char IsInQsand;
	
	short IsClimbing;
	
	short Height;
	
	short Height2;//sprite height

	
//	short Maskbase;
//	short Spritebase;//small left, small right, large left, large right, fire left, fire  right
//	short Spritenr;
	short SpriteOffset;
	short Blit;
	
	short Offset;
//	short PrevCompX;
//	short PrevCompY;
	

	short Test;
	short Test2;

	
	//fly code
	short Flycount;
	short Runcount;
	
	
//	short MapX;
//	short MapY;
	
//	char IsOnMapBoat;
//	char IsClouded;
	
	char Passified;
	
//	char Itemlist[itemlist_length];
	//0: empty slot
	//1: mushrom
	//2: flower
	//3: leaf
	//4: star
	//5: whistle (warp zone)
	//6: hammer (map)
	//7: P-wing
	//8: 
	//9: 
	//10: 
	
	short Curr_item;
//	char Cards[3];

//	char CurrCard;

	short Behind_fg;
	short White_magic_box_counter;

	short RacoonTaleAnim;

}player;

typedef struct {
		
	unsigned short Score;
	
	//are those neccessarry???
	short Spritebase;//small left, small right, large left, large right, fire left, fire  right
	short Spritenr;
	short Maskbase;
	
	short PrevCompX;
	short PrevCompY;
	
	short MapX;
	short MapY;
	
	short MapBorderLeft;//bugfix
	short MapBorderRight;
	
	short L;
	
	char Life;
	char Lives;
	
	char Attribs;
									//bit7: set when mario is "super",has got a star.
									//bit6 set: Can throw fireballs.
									//bit5 set: has the racoon suit.
									//bit4 set: Draw player before foreground, used in some effects		
									//bit3 set: P-wing: Always able to fly
	
	char Coins;	

	char IsOnMapBoat;
	char IsClouded;
	
	char MapColor;
	
	char Cards[3];
	char CurrCard;
	
	char Itemlist[itemlist_length];
		
}saveplayer;

extern player Player;
extern saveplayer SavePlayer;

extern shot Player_smallshots[nr_of_player_smallshots];


extern unsigned short *Mariosprites;
extern unsigned short *Mariomasks;



extern void (*Bonushandlers[20])( short X, short Y);//pointers to bonustiles functions
extern void (*Interactive_tile_handlers[47])( short X, short Y);//pointers to interactive tiles functions

extern void (*Interactive_non_solid_tile_handlers[21])( short X, short Y);//pointers to inatective tiles functions


void Playerinit();

void Handleplayer();



void Player_die();
void Player_die_hard();
void Player_die_anim();

unsigned char Get_tile(short X,short Y);


void Put_tile(short X,short Y,unsigned char Tile);


void Set_player_attribs();
void Set_player_attribs_right(unsigned char *Attribs);   
void Set_player_attribs_left(unsigned char *Attribs);
void Set_player_attribs_up(unsigned char *Attribs);
void Set_player_attribs_down(unsigned char *Attribs);


void Dummy_func( short X, short Y);



void Player_handle_fireballs();

void Player_add_fireball();


void Player_fireball_handler(shot *Fireball);


#endif