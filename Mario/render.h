
#ifndef __render__
#define __render__

//ti 89 and ti 89T spesific code
#ifdef PRODUCE_TI89_CODE
//#if TI89

#define drawmode_bg TM_GRPLC89
#define drawmode_fg TM_GOR89
#define drawmode_fg_m TM_MASK89


#define scroll_limit_left		70
//50
#define scroll_limit_right	90
//110
#define scroll_limit_up			35
//30
#define scroll_limit_down		65
//70

#define screen_width			 160
#define screen_height			 100

#define statusbar_offset 2760

#define screen_offset_sg_x		0
#define screen_offset_sg_y		0

#endif

//ti 92+ and ti v200 spesific code
//#ifndef PRODUCE_TI89_CODE
#ifdef PRODUCE_V200_CODE
//#if V200

#define scroll_limit_left		110
#define scroll_limit_right	130
#define scroll_limit_up			49
#define scroll_limit_down		79

#define screen_width			 240
#define screen_height			 127

#define drawmode_bg TM_GRPLC
#define drawmode_fg TM_GOR
#define drawmode_fg_m TM_MASK

#define statusbar_offset 3600

#define screen_offset_sg_x		40
#define screen_offset_sg_y		14

#endif

//#if TI92PLUS
#ifdef PRODUCE_TI92PLUS_CODE

#define scroll_limit_left		110
#define scroll_limit_right	130
#define scroll_limit_up			49
#define scroll_limit_down		79

#define screen_width			 240
#define screen_height			 127


#define drawmode_bg TM_GRPLC
#define drawmode_fg TM_GOR
#define drawmode_fg_m TM_MASK

//#define statusbar_offset 3600
#define statusbar_offset 3600

#define screen_offset_sg_x		40
#define screen_offset_sg_y		14

#endif

extern short Update_FG;

extern short BgX,BgY,FgX,FgY;		//screen render points,PROBABLY UNNESSESSARRY
extern Plane Bg_plane;
extern AnimatedPlane Fg_plane,Fg_mask,Map_plane;
extern void *Buffer;
//extern unsigned short *Tiles;

extern void *Statusbar;
extern unsigned char StatBarHeight;

extern void *dBufHPL_G, *dBufHPD_G;

void GrayDBufToggleSync_SetPointers();

void DrawString(short X,short Y,const char* Str,short Attr,short Font);

void Render();

void DrawMarioCursor(short X,short Y);

void DrawSprite16SMASKR(short x asm("%d0"),short y asm("%d1"),short h asm("%d2"),const unsigned short *sprt0,const unsigned short *sprt1,const unsigned short *mask);

void GrayOutlineRect(short X1 asm("%d0"),short Y1 asm("%d1"),short X2 asm("%d2"),short Y2 asm("%d3"),short Color);

void FilledRectDark(short X1 asm("%d0"),short Y1 asm("%d1"),short X2 asm("%d2"),short Y2 asm("%d3"));

void FilledRectEraseLight(short X1 asm("%d0"),short Y1 asm("%d1"),short X2 asm("%d2"),short Y2 asm("%d3"));

void FilledRectEraseDark(short X1 asm("%d0"),short Y1 asm("%d1"),short X2 asm("%d2"),short Y2 asm("%d3"));


//void Draw_player();	//the following commented out functions are inlined manually in Render()!
	
//void Draw_enemies();
	
//void Draw_items();

//void Draw_shells();

//void Draw_items();

//void Draw_objects();



void Adjust_renderpoint();//scrolling hysteresis


void Draw_brick_fragments(object *Object);



void Draw_killed_fireballs(object *Object);

void Draw_elastic_tiles(object *Object);

void Draw_bb_coin(object* Object);

void Draw_score(object* Object);

void Draw_collided_shells(object* Object);

void Draw_climbing_flower(object* Object);

void Draw_killed_cannonballs(object* Object);

void Draw_splash(object* Object);



//void UpsideDownGrayClipSprite16_MASK_R(register short x asm("%d0"),register short y asm("%d1"),register short h asm("%d2"),unsigned short *sprt0,unsigned short *sprt1,unsigned short *mask0,unsigned short *mask1,register void *dest0 asm("%a0"),register void *dest1 asm("%a1")) __attribute__((__stkparm__));

//void GrayClipSprite16_SMASKBLIT_R(short x asm("%d0"),short y asm("%d1"),short h asm("%d2"),unsigned short *sprt0,unsigned short *sprt1,unsigned short *mask,unsigned short maskval asm("%d3"),void *dest0 asm("%a0"),void *dest1 asm("%a1")) __attribute__((__stkparm__));

void Update_statusbar(/*char Force_update*/);

void Draw_statusbar();

void Draw_player();


void DrawItem(short X,short Y,short Item);

void Render_map();

#endif