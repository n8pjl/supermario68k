
/*
objects.c:
those variables and functions deals with all kind of objects in the game that
doesn't come uner the categories Tiles, enemies, shots/fireballs, shells,
player, etc.

An object can be: fragments of crushed bricks, dead enemy flying through the
air, animations etc,
*/

#include "objects.h"
#include "items.h"
#include "render.h"

struct object *Objects;

/*
inline void Handle_objects(){
        short C;

        for(C=0;C<nr_of_objects;C++){
                if(Objects[C].Active)
                        Objects[C].Handler(&Objects[C]);
        };
};
*/

/*
void Handle_brick_fragments(object *Object){

        if((++Object->Active) == 36)//Xpos
                Object->Active = 0;

        if(Object->Active >= 10){
                Object->Data0--;//low y
        }
        else{
                Object->Data0++;//low y
        };
        if(Object->Active<15){
                Object->Data1 += 1;//high y
        }
        else{
                Object->Data1 --;//high y
        };
};
*/
/*
void Handle_drop_down(object *Object){
        if((Object->Y += 3)>=FgY+screen_height){
                Object->Active = 0;
        };
};*/
/*
void Handle_killed_fireball(object *Object){
        Object->Active++;

        if(Object->Active == 4){
                Object->Data0 = 16;
        }
        else{
                if(Object->Active == 7){
                        Object->Data0 = 32;
                }
                else{
                        if(Object->Active == 10)
                                Object->Active = 0;
                };
        };

};*/

void Handle_timed_coinconvert(struct object *Object) {
  Object->X++;

  if (Object->X >= pow_item_time) {
    Convert_brick_coin();
    Object->Active = 0;
  };
};

/*
void Handle_elastic_tile(object *Object){//data2: b7:up b6:down b5:left b4:right

        //Object->Active--;
        if( (--Object->Active)==0 ){
                //*(
(char*)(Fg_plane.p.matrix+(Object->Y/16)*Fg_plane.p.width+(Object->X/16)) ) =
Object->Data0; Put_tile(Object->X,Object->Y,Object->Data0);
                Fg_plane.p.force_update=1;
                Fg_mask.p.force_update=1;
        };
};
*/
/*
void Handle_bb_coin(object* Object){
        Object->Active--;

        if(Object->Active>15){
                Object->Y-=5;
        }
        if( (Object->Active<=15) && Object->Active>6){
                Object->Y+=5;
        };
        if(Object->Active<=6){
                if(Object->Active>3){
                        Object->Data0 = 24*4;//16*4
                }
                else{
                        Object->Data0 = 24*5;//16*5
                        Object->Data1 = 20;
                };
        }
        else{
                Object->Data0 = 24*Fg_plane.step;//16
        };

};
*/
/*
void Handle_score(object* Object){
        Object->Active--;
        Object->Y--;
};
*/
/*
void Handle_climbing_flower(object* Object){

        if(Object->Data0<16){//coming out of box
                Object->Data0+=2;
                Object->Y-=2;

        }
        else{

                if(Free_up(Object->X,Object->Y)){
                        Object->Y -= 2;//grow
                        //*(
(char*)(Fg_plane.p.matrix+(((Object->Y+Object->Data0-1)/16))*Fg_plane.p.width+(Object->X/16))
)=ladder; Put_tile(Object->X,Object->Y+Object->Data0-1,ladder);
                        if(!(Object->Y%16)){
                        //if(!(Object->Y&0x000f)){
                                Fg_plane.p.force_update=1;
                                Fg_mask.p.force_update=1;
                        };
                }
                else{
                        Object->Active = 0;//suicide
                        //*(
(char*)(Fg_plane.p.matrix+(((Object->Y)/16))*Fg_plane.p.width+(Object->X/16))
)=ladder;
                };

        }

};
*/
void Splash(short X, short Y) {
  short C;

  if ((C = Find_free_object()) != -1) {

    Objects[C].X = X;
    Objects[C].Y = Y;
    Objects[C].Data0 = 7;
    Objects[C].Data1 = 0;
    Objects[C].Active = 25;
    //(object*)Objects[C].Handler = Dummy_func_;
    Objects[C].Draw = Draw_splash;
  }
};

short Find_free_object() {
  short C;

  for (C = 0; C < nr_of_objects; C++) {
    if (!(Objects[C].Active)) {
      return C;
    };
  };
  return -1;
}
