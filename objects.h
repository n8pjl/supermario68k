#pragma once

#include <stdint.h>
#define nr_of_objects 10

struct object {
  int16_t X;
  int16_t Y;

  /*	char Data0;
          char Data1;
          char Data2;
          char Active;*/
  int16_t Data0;
  int16_t Data1;
  int16_t Data2;
  int16_t Active;
  //	void (*Handler)(void *Object);
  void (*Draw)(
      struct object *Object); // the function that draws the spesific object
};

// extern object Objects[nr_of_objects];
extern struct object *Objects;

// void Handle_objects();
// Using a macro to inline this func saved some bytes
// Now redundant: Everything is now handled at the same time as drawing objects
/*
#define Handle_objects() {\
        short C;\
        for(C=0;C<nr_of_objects;C++){\
                if(Objects[C].Active)\
                        Objects[C].Handler(&Objects[C]);\
        };\
};\
*/
// void Handle_brick_fragments(object *Object);

// void Handle_drop_down(object *Object);

// void Handle_killed_fireball(object *Object);

void Handle_timed_coinconvert(struct object *Object);

// void Handle_elastic_tile(object *Object);

// void Handle_bb_coin(object* Object);

// void Handle_score(object* Object);

// void Handle_climbing_flower(object* Object);

void Splash(int16_t X, int16_t Y);

int16_t Find_free_object();
