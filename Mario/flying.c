


#include "all.h"

flying_platform* Flying_platforms;



inline void Handle_flying_platforms(){
	short C;
	
	for(C=0;C<Leveldata.Nr_of_flying_platforms;C++){
		
		if(Flying_platforms[C].Active){
			
			Flying_platforms[C].Handler(&Flying_platforms[C]);
			
			if( (Flying_platforms[C].X>(Leveldata.Width*16)) || ((Flying_platforms[C].X+Flying_platforms[C].Width*16)<0) || (Flying_platforms[C].Y>(Leveldata.Height*16)) || ((Flying_platforms[C].Y+16)<0) )
				Flying_platforms[C].Active = 0;
			
		};		
		
	};	
	
};



void Platform_handler_1(flying_platform* Platform){
	//this one handles all basic platform operations, such as motion and player squeezed to death etc.
	//Data2 is speed in x dir
	//Data3 is speed in y dir	
	//Data0 is motion length before change dir
																		 
	char P_attribs[6];
	//short C;
	
	if((++Platform->Active)==3){
		Platform->Active = 1;
//	if(Fg_plane.frame%2){	
	
		Platform->X += Platform->Data2;
		Platform->Y += Platform->Data3;
		
		//New V 1.02: Corrected FgX to FgY in the line below. Fixed a bug where some flying platforms dissappeared too early
		//New V 1.04: Added && (Platform->Data3!=0): Preventing killing platforms that aren't moving
		if( (Platform->Data0==0) && (Platform->Data3!=0) && (Platform->Y>(FgY+screen_height+32)) ){//if falling with no return and outside active area
			Platform->Active = 0;//kill platform
		}
		
		if(BoundsCollide2(Player.X,Player.Y,Player.Height-1,15,Platform->X,Platform->Y,16,Platform->Width*16)){
			if(Platform->PlayerOn == 0){
				Platform->PlayerOn = 2;
			};
		};	
				
	
		if(Platform->PlayerOn){
		
			
			if(Platform->Data2){
				
		/*		if(Platform->Data2>0)
					Set_player_attribs_right(P_attribs);
				if(Platform->Data2<0)//else
					Set_player_attribs_left(P_attribs);
		*/

				if(Platform->Data2>0){
					//New: V 1.04 Changed from using Set_attribs func to Free_dir func.
					// This Fixed bug where player died on contact with flying platforms when he was positioned on slope tiles
					// Mario is now pushed to the side and upwards instead of dieing when he is positioned on slope tiles
					
					//Set_player_attribs_right(P_attribs);					
					P_attribs[5] = Free_right(Player.X,&Player.Y,Player.Height,Platform->Data2);
				}
				else{
					//New: V 1.04 Changed from using Set_attribs func to Free_dir func.
					// This Fixed bug where player died on contact with flying platforms when he was positioned on slope tiles
					// Mario is now pushed to the side and upwards instead of dieing when he is positioned on slope tiles

					//Set_player_attribs_left(P_attribs);
					P_attribs[5] = Free_left(Player.X,&Player.Y,Player.Height,Platform->Data2);
				}	
				
				if(P_attribs[5]>=abs(Platform->Data2)){
					Player.X += Platform->Data2;					
				};
			};	
			
			if(Platform->Data3 && (Platform->PlayerOn != 2 ) ){
					
				if(Platform->Data3>0){
					char Temp = Player.IsInWater;
					Player.IsInWater = 0;
					Set_player_attribs_down(P_attribs);
					Player.IsInWater = Temp;
				}
					
				if(Platform->Data3<0)//else
					Set_player_attribs_up(P_attribs);
					
				if(P_attribs[5]>=abs(Platform->Data3)){
					Player.Y += Platform->Data3;
				};
				
			};
				
			Platform->PlayerOn = 0;
		};
	
		if(Platform->Data2){
			Platform->Data1 += Platform->Data2;
		}
		else{
			Platform->Data1 += Platform->Data3;
		};
	
		if( (Platform->Data0 && ((Platform->Data1 == 0) || (Platform->Data1 == Platform->Data0)) ) ){
			Platform->Data2 = -Platform->Data2;
			Platform->Data3 = -Platform->Data3;
		};
			
	
	 if(BoundsCollide2(Player.X,Player.Y,Player.Height-1,15,Platform->X,Platform->Y,16,Platform->Width*16))
 			Player_die_hard();
	
	};
	
};

void Platform_handler_2(flying_platform* Platform){
//handles platforms that starts to fall when player lands on it
//Data0 and Data1 MUST initially be set to 0 !!! or else....

	if( (Platform->PlayerOn == 3) && !(Platform->Data3) ){//player has landed on me?
		//start to fall
		Platform->Data3 = 2;
	};
	
		
	Platform_handler_1(Platform);
	
};


void Platform_handler_3(flying_platform* Platform){
	
	//if((Platform->Active)<40){//shake time
	if((Platform->Data1)<40){//shake time
		Platform->Data1++;
		//Platform->Active++;
		//shake
		if(BoundsCollide2(Player.X,Player.Y,Player.Height,15,Platform->X,Platform->Y-4,16,Platform->Width*16)){
			//Platform->Y += (Platform->Data1%2?-2:2);
	/*		if( (Platform->Data1%4)==1 ){
				Platform->Y -= 2;
			}
			if( (Platform->Data1%4)==3 ){
				Platform->Y += 2;
			}*/
			switch(Platform->Data1%4){
				case 1:Platform->Y -= 2;
				break;
				case 3:Platform->Y += 2;
				break;
				
			}
			//Platform->Y += (Fg_plane.step%2?-1:1);
			//while(1);
		}
		else{
			Platform->Active = 0;
			Put_tile(Platform->X,Platform->Y,falling_block);
			Update_FG = 1;//Fg_plane.p.force_update = Fg_mask.p.force_update = 1;
		}
		
	}
	else{
		//fall
		//Platform->Data0 = Platform->Data1 = 0;
	//	if(Platform->Active==40){
			Put_tile(Platform->X,Platform->Y,0);//delete tile, start to fall
			Platform->Data3 = 2;//y speed
			Platform->Data0 = 0;
			Platform->Active = 1;
//			Platform->DrawMode = 1;
			Platform->Handler = Platform_handler_1;
	//	}
		
		//Platform_handler_1(Platform);
	}
	
	//Platform_handler_1(Platform);//handles fall and PlayerIsOn
	/*
	if(Platform->Data3){//jalla
		Platform->Y += Platform->Data3;
	}
	*/
};

