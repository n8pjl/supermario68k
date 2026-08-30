#pragma once

#include <stdint.h>
#include "compat/tilemap.h"
#include "version.h"
#include "objects.h"

extern int16_t scroll_limit_left;
extern int16_t scroll_limit_right;
extern int16_t scroll_limit_up;
extern int16_t scroll_limit_down;

extern int16_t screen_width;
extern int16_t screen_height;

#define drawmode_bg TM_GRPLC
#define drawmode_fg TM_GOR
#define drawmode_fg_m TM_MASK

extern int16_t statusbar_offset;

extern int16_t screen_offset_sg_x;
extern int16_t screen_offset_sg_y;

extern int16_t Update_FG;

extern int16_t BgX, BgY, FgX, FgY; // screen render points,PROBABLY UNNESSESSARRY
extern struct Plane Bg_plane;
extern struct AnimatedPlane Fg_plane, Fg_mask, Map_plane;
extern void *Buffer;
// extern unsigned short *Tiles;

extern void *Statusbar;
extern uint8_t StatBarHeight;

extern void *dBufHPL_G, *dBufHPD_G;

void init_calc_screen_constants(bool ti89_mode);

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
