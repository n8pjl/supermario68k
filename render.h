#pragma once

#include <stdint.h>
#include "compat/tilemap.h"
#include "objects.h"

// ti 89 and ti 89T spesific code
#ifdef PRODUCE_TI89_CODE
// #if TI89

#define drawmode_bg TM_GRPLC89
#define drawmode_fg TM_GOR89
#define drawmode_fg_m TM_MASK89

#define scroll_limit_left 70
// 50
#define scroll_limit_right 90
// 110
#define scroll_limit_up 35
// 30
#define scroll_limit_down 65
// 70

#define screen_width 160
#define screen_height 100

#define statusbar_offset 2760

#define screen_offset_sg_x 0
#define screen_offset_sg_y 0

#endif

// ti 92+ and ti v200 spesific code
// #ifndef PRODUCE_TI89_CODE
#ifdef PRODUCE_V200_CODE
// #if V200

#define scroll_limit_left 110
#define scroll_limit_right 130
#define scroll_limit_up 49
#define scroll_limit_down 79

#define screen_width 240
#define screen_height 127

#define drawmode_bg TM_GRPLC
#define drawmode_fg TM_GOR
#define drawmode_fg_m TM_MASK

#define statusbar_offset 3600

#define screen_offset_sg_x 40
#define screen_offset_sg_y 14

#endif

// #if TI92PLUS
#ifdef PRODUCE_TI92PLUS_CODE

#define scroll_limit_left 110
#define scroll_limit_right 130
#define scroll_limit_up 49
#define scroll_limit_down 79

#define screen_width 240
#define screen_height 127

#define drawmode_bg TM_GRPLC
#define drawmode_fg TM_GOR
#define drawmode_fg_m TM_MASK

// #define statusbar_offset 3600
#define statusbar_offset 3600

#define screen_offset_sg_x 40
#define screen_offset_sg_y 14

#endif

extern int16_t Update_FG;

extern int16_t BgX, BgY, FgX, FgY; // screen render points,PROBABLY UNNESSESSARRY
extern struct Plane Bg_plane;
extern struct AnimatedPlane Fg_plane, Fg_mask, Map_plane;
extern void *Buffer;
// extern unsigned short *Tiles;

extern void *Statusbar;
extern uint8_t StatBarHeight;

extern void *dBufHPL_G, *dBufHPD_G;

void GrayDBufToggleSync_SetPointers();

void DrawString(int16_t X, int16_t Y, const char *Str, int16_t Attr, int16_t Font);

void Render();

void DrawMarioCursor(int16_t X, int16_t Y);

void DrawSprite16SMASKR(int16_t x, int16_t y, int16_t h, const uint16_t *sprt0,
                        const uint16_t *sprt1,
                        const uint16_t *mask);

void GrayOutlineRect(int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, int16_t Color);

void FilledRectDark(int16_t X1, int16_t Y1, int16_t X2, int16_t Y2);

void FilledRectEraseLight(int16_t X1, int16_t Y1, int16_t X2, int16_t Y2);

void FilledRectEraseDark(int16_t X1, int16_t Y1, int16_t X2, int16_t Y2);

// void Draw_player();	//the following commented out functions are inlined
// manually in Render()!

// void Draw_enemies();

// void Draw_items();

// void Draw_shells();

// void Draw_items();

// void Draw_objects();

void Adjust_renderpoint(); // scrolling hysteresis

void Draw_brick_fragments(struct object *Object);

void Draw_killed_fireballs(struct object *Object);

void Draw_elastic_tiles(struct object *Object);

void Draw_bb_coin(struct object *Object);

void Draw_score(struct object *Object);

void Draw_collided_shells(struct object *Object);

void Draw_climbing_flower(struct object *Object);

void Draw_killed_cannonballs(struct object *Object);

void Draw_splash(struct object *Object);

// void UpsideDownGrayClipSprite16_MASK_R(register short x,register short
// y,register short h,unsigned short *sprt0,unsigned short *sprt1,unsigned short
// *mask0,unsigned short *mask1,register void *dest0,register void *dest1)
// __attribute__((__stkparm__));

// void GrayClipSprite16_SMASKBLIT_R(short x,short y,short h,unsigned short
// *sprt0,unsigned short *sprt1,unsigned short *mask,unsigned short maskval,void
// *dest0,void *dest1) __attribute__((__stkparm__));

void Update_statusbar(/*char Force_update*/);

void Draw_statusbar();

void Draw_player();

void DrawItem(int16_t X, int16_t Y, int16_t Item);

void Render_map();
void DrawBg(int16_t X, int16_t Y);
