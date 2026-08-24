
#include "all.h"


//shell Shells[max_nr_of_shells];
shell* Shells;


inline void Player_bounching_shell_hadler(){
	short A,C,D;
	unsigned char Temp;
	short Dist=0;
	
	//New: V 1.04 Added a function that adds killed shells, instead of repeated code. Saved some bytes
	void Add_killed_shell(shell* Shell,short DirSpeed){
		// Possible optimization: shange DirSpeed parameter to the 2nd X coordinate that is used to determine the direction
		short O;
		if( (O=Find_free_object())!=-1){
			Objects[O].Active = Shell->Active;			
			Objects[O].Data0 = DirSpeed ;
			Objects[O].X = Shell->X;
			Objects[O].Y = Shell->Y;							
			(object*)Objects[O].Draw = Draw_collided_shells;
		};
		Shell->Active = 0;
	};
	
	
	
	for(C=0;C<max_nr_of_shells;C++){
		shell* Shell = &Shells[C];
		if(/*Shells[C].*/Shell->Active){//test if Active(existing)

			if(/*Shells[C].*/Shell->Count)
				/*Shells[C].*/Shell->Count--;

			if( (/*Shells[C].*/Shell->X>(FgX-64)) && (/*Shells[C].*/Shell->X<(FgX+BIG_VSCREEN_WIDTH+64)) && (/*Shells[C].*/Shell->Y>(FgY-64)) && (/*Shells[C].*/Shell->Y<(FgX+BIG_VSCREEN_HEIGHT+64)) ){//in active area ?
			
				if(/*Shells[C].*/Shell->Dir>0){//dir = right ?
				
					//if( abs(Shells[C].Dir)>Free_right(Shells[C].X,&Shells[C].Y,16,Shells[C].Dir) ){//not free right
					Dist = Free_right(/*Shells[C].*/Shell->X,&/*Shells[C].*/Shell->Y,16,/*Shells[C].*/Shell->Dir);
					if( Dist <= 0 ){//not free right
									
						Temp=Get_tile(/*Shells[C].*/Shell->X+15+/*Shells[C].*/Shell->Dir,/*Shells[C].*/Shell->Y);			
						if( (Temp>=solid_interactive_bounch_low) && (Temp<=solid_interactive_bounch_high) ){
							Interactive_tile_handlers[Temp-solid_interactive_low](/*Shells[C].*/Shell->X+15+/*Shells[C].*/Shell->Dir,/*Shells[C].*/Shell->Y);
						};
						
						Temp=Get_tile(/*Shells[C].*/Shell->X+15+/*Shells[C].*/Shell->Dir,/*Shells[C].*/Shell->Y+15);
						if( (Temp>=solid_interactive_bounch_low) && (Temp<=solid_interactive_bounch_high) ){
							Interactive_tile_handlers[Temp-solid_interactive_low](/*Shells[C].*/Shell->X+15+/*Shells[C].*/Shell->Dir,/*Shells[C].*/Shell->Y+15);
						};
						
						/*Shells[C].*/Shell->Dir = -shell_speed;					
						
					}
					else{//free right
						//Shells[C].X += Shells[C].Dir;
						/*Shells[C].*/Shell->X += min(/*Shells[C].*/Shell->Dir,Dist);
						
						if( (/*Shells[C].*/Shell->X)>16*(short)Fg_plane.p.width)//test if outside level (right)
							/*Shells[C].*/Shell->Active = 0;//kill shell
					};
				}//>0
				else{//dir = left
					Dist = Free_left(/*Shells[C].*/Shell->X,&/*Shells[C].*/Shell->Y,16,/*Shells[C].*/Shell->Dir);
					//if( abs(Shells[C].Dir)>Free_left(Shells[C].X,&Shells[C].Y,16,Shells[C].Dir)/*(Get_tile(Shells[C].X-Shells[C].Dir-3 ,Shells[C].Y)>=solid_low) && (Get_tile(Shells[C].X-Shells[C].Dir-3,Shells[C].Y+15)>=solid_low)*/ ){//not free left
				if( Dist <=0 ){//not free left
						
						Temp=Get_tile(/*Shells[C].*/Shell->X+/*Shells[C].*/Shell->Dir-3,/*Shells[C].*/Shell->Y);
						if( (Temp>=solid_interactive_bounch_low) && (Temp<=solid_interactive_bounch_high) ){
							Interactive_tile_handlers[Temp-solid_interactive_low](/*Shells[C].*/Shell->X+/*Shells[C].*/Shell->Dir-3,/*Shells[C].*/Shell->Y);
						};
						
						Temp=Get_tile(/*Shells[C].*/Shell->X+/*Shells[C].*/Shell->Dir-3,/*Shells[C].*/Shell->Y+15);
						if( (Temp>=solid_interactive_bounch_low) && (Temp<=solid_interactive_bounch_high) ){
							Interactive_tile_handlers[Temp-solid_interactive_low](/*Shells[C].*/Shell->X+/*Shells[C].*/Shell->Dir-3,/*Shells[C].*/Shell->Y+15);
						};
						
						/*Shells[C].*/Shell->Dir = shell_speed;
					}
					else{//free left
						//Shells[C].X += Shells[C].Dir;
						/*Shells[C].*/Shell->X += max(/*Shells[C].*/Shell->Dir,-Dist);
						
						if(/*Shells[C].*/Shell->X < -16)//test if outside level (left)
							/*Shells[C].*/Shell->Active = 0;//kill shell
					};
				};
				
				//gravity	
				/*if( (Get_tile(Shells[C].X,Shells[C].Y+16+15)>=solid_down_low) || (Get_tile(Shells[C].X+15,Shells[C].Y+16+15)>=solid_down_low) ){//free down ?	
					Temp = (15-(Shells[C].Y+15)%16);//distance to next tile down
					}
					else{
						Temp = 16;
				};*/
				Temp = Free_down(/*Shells[C]*/Shell->X,/*Shells[C].*/Shell->Y+16);//free down ?	

				
				

				/*Shells[C].*/Shell->Y += (Temp>enemy_fallspeed) ?  enemy_fallspeed:Temp;//fall
				
				
				
				
				//test if outside level (bottom)
				if(/*Shells[C].*/Shell->Y>=(Leveldata.Height*16))
					/*Shells[C].*/Shell->Active = 0;
			
				//test if collide with player
				//change this to use testcollide with different heights
				//if( !(Shells[C].Count) && (TestCollide16(Player.X,Player.Y,Shells[C].X,Shells[C].Y,16,Mariosprites+2*Marioanimtab[Player.Spritebase][Player.Spritenr],shell_0_sprite) /*&&*/|| TestCollide16(Player.X,Player.Y+Player.Height-16,Shells[C].X,Shells[C].Y,16,Player.Sprite+Player.Height-16,shell_0_sprite) ) ){
				if(!(/*Shells[C].*/Shell->Count) && TestCollide162h_R(Player.X,Player.Y,/*Shells[C].*/Shell->X,/*Shells[C].*/Shell->Y,Player.Height,16,Mariosprites+2*Marioanimtab[SavePlayer.Spritebase][SavePlayer.Spritenr],shell_0_sprite)){	
					if(SavePlayer.Attribs & 0b10000000){//mario star?
						//New: V 1.04 Added missing animation when a shell is killed by "Star" Mario
						Add_killed_shell(Shell,(Player.X>Shell->X) ? -4:4);
					}
					else{//player can die
					
						if( Player.IsFalling && (Player.Y < /*Shells[C].*/Shell->Y) ){//player jumped on shell
							
							//if( ((Player.X>=Shells[C].X) && (Shells[C].Dir>0)) || ((Player.X<=Shells[C].X) && (Shells[C].Dir<0)) ){
							
							/*Shells[C].*/Shell->Active = 0;
							/*Shells[C].*/Shell->Enemy->Attribs = 0b11100010;
							(enemy *)(/*Shells[C].*/Shell->Enemy->Handler) = Enemy_handler_6;
							/*Shells[C].*/Shell->Enemy->Life = /*Shells[C].*/Shell->Enemy->Mode;
							/*Shells[C].*/Shell->Enemy->Mode = 60;
							/*Shells[C].*/Shell->Enemy->X = /*Shells[C].*/Shell->X;
							/*Shells[C].*/Shell->Enemy->Y = /*Shells[C].*/Shell->Y;
							
							/*if( (Shells[C].Enemy->Sprite == turtle_right_sprite) || (Shells[C].Enemy->Sprite == turtle_left_sprite) ){
								Shells[C].Enemy->Life = 1;
							}
							else{
								Shells[C].Enemy->Life = 2;
							};*/							
							
							(enemy *)(/*Shells[C].*/Shell->Enemy->Die) = Enemy_die_5;
							
							Player.IsJumping = 0.75*jumpheight;//enable rejump
							Player.Yoffset = -2;
							Player.Yoffset = (Player.X>/*Shells[C].*/Shell->X ? 2:-2);
							//};
							
						}
						else{
							Player_die();
						};
					
						
					};
				};
				
				
				//test if collide with each other
				for(D=0;D<max_nr_of_shells;D++){
					if(C!=D){
						shell* Shell2 = &Shells[D];
						if( BOUNDS_COLLIDE16(/*Shells[C].*/Shell->X,/*Shells[C].*/Shell->Y,/*Shells[D].*/Shell2->X,/*Shells[D].*/Shell2->Y) && /*Shells[D].*/Shell2->Active ){
							
							//animate
							//OPOTIMIZE THIS !!!!!!
							/*if( (A=Find_free_object())!=-1){
								Objects[A].Active = Shell->Active;
								Shell->Active = 0;//kill shell
								Objects[A].Data0 = (Shell2->X>Shell->X) ? -4:4 ;
								Objects[A].X = Shell->X;
								Objects[A].Y = Shell->Y;
								(object*)Objects[A].Draw = Draw_collided_shells;
							};
							if( (A=Find_free_object())!=-1){
								Objects[A].Active = Shell2->Active;
								Shell2->Active = 0;//kill shell
								Objects[A].Data0 = (Shell->X>Shell2->X) ? -4:4 ;
								Objects[A].X = Shell2->X;
								Objects[A].Y = Shell2->Y;								
								(object*)Objects[A].Draw = Draw_collided_shells;
							};*/
//							Shells[C].Active = 0;//kill shell
//							Shells[D].Active = 0;
							
							//New: V 1.04 Function call instead of repeated code saved bytes
							Add_killed_shell(Shell,Shell2->X>Shell->X ? -4:4 );
							Add_killed_shell(Shell2,Shell->X>Shell2->X ? -4:4 );
						};
					};						

				};
				
				//draw the shell
				//GrayClipSprite16_MASK_R(Shells[C].X-FgX,Shells[C].Y-FgY,/*Shells[C].Height*/16,/*Shells[C].Sprite*/shell_0_sprite,/*Shells[C].Sprite+Shells[C].Height*/shell_0_sprite+16,/*Shells[C].Mask*/shell_0_mask,/*Shells[C].Mask*/shell_0_mask,GrayDBufGetHiddenPlane(LIGHT_PLANE),GrayDBufGetHiddenPlane(DARK_PLANE));
				
			}//end of active area
			else{//outside active area
				/*Shells[C].*/Shell->Active = 0;//remove shell
			};
		};//end of active
	};//end for
}



void Add_shell(/*short X,short Y,*/short Dir,char Type, enemy *Enemy){
	short C;
	
	
	for(C=0;C<max_nr_of_shells;C++){//loop through array of shells
		if( !(Shells[C].Active) ){//free space ?
			shell* Shell = &Shells[C];
			
			/*Shells[C].*/Shell->X = Enemy->X;//add shell
			/*Shells[C].*/Shell->Y = Enemy->Y;
			/*Shells[C].*/Shell->Active = Type;
			/*Shells[C].*/Shell->Dir = Dir;
			/*Shells[C].*/Shell->Enemy = Enemy;
			/*Shells[C].*/Shell->Count = 4; //can't kill mario next 4 frames 8/shellspeed+1
			break;//C=max_nr_of_shells;//exit loop
		};
	};
	
};
/*
void Handle_collided_shells(object* Object){
	Object->Active++;
	
	if(Object->Active<3){
		Object->Y -= 4;
	}
	else{
		if(Object->Active>6)
			Object->Y += 4;
	};
	
	Object->X += Object->Data0;	//direction
	
	if( Object->Active > 28 )//with speed 4 in y dir this nr ensures that none is removed before they have left the screen
		Object->Active = 0;//Enemy_remove(Enemy);
};*/


