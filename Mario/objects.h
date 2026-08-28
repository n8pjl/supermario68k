
#ifndef __objects__
#define __objects__

#define nr_of_objects		10



typedef struct object {
	short X;
	short Y;

/*	char Data0;
	char Data1;
	char Data2;
	char Active;*/
	short Data0;
	short Data1;
	short Data2;
	short Active;
//	void (*Handler)(void *Object);
	void (*Draw)(struct object *Object);//the function that draws the spesific object
}object;




//extern object Objects[nr_of_objects];
extern object* Objects;


//void Handle_objects();
//Using a macro to inline this func saved some bytes
//Now redundant: Everything is now handled at the same time as drawing objects
/*
#define Handle_objects() {\
	short C;\
	for(C=0;C<nr_of_objects;C++){\
		if(Objects[C].Active)\
			Objects[C].Handler(&Objects[C]);\
	};\
};\
*/
//void Handle_brick_fragments(object *Object);


//void Handle_drop_down(object *Object);

//void Handle_killed_fireball(object *Object);

void Handle_timed_coinconvert(object *Object);

//void Handle_elastic_tile(object *Object);

//void Handle_bb_coin(object* Object);

//void Handle_score(object* Object);

//void Handle_climbing_flower(object* Object);

void Splash(short X,short Y);


short Find_free_object();

#endif