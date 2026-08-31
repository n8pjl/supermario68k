#pragma once

// misc stuff used by compression system

enum Enemy_compression_model_limits {
	handler1_users_low = 1,
	handler1_users_high = 20,

	flowers_low = 23,
	flowers_high = 30,
	flower1_low = 23,
	flower1_high = 26,
	flower2_low = 27,
	flower2_high = 30,

	boomerang_guy_users_low = 31,
	boomerang_guy_users_high = 42,

};

// constants might change.....
enum Enemy_compression_models {
	goomba_free = 1,
	goomba_limited = 2,
	turtle_free = 3,
	turtle_limited = 4,
	hedgehog_free = 5,
	hedgehog_limited = 6,
	beetle_free = 7,
	beetle_limited = 8,
	fish_free = 9,
	fish_limited = 10,
	skel_tur_free = 11,
	skel_tur_limited = 12,

	flying_goomba = 61,
	flying_fish = 60,

	flying_turtle = 58,

	bomb = 63,

	horizontal_cannon = 52,
	diagonal_cannon = 53,

	lavaball = 47,

	ghost_limited = 55,

	flower1_upside_down = 25,
	flower1 = 26,
	flower2_upside_down = 29,
	flower2 = 30,

	boomerang_guy_s = 31,
	boomerang_guy_b_t = 32,
	boomerang_guy_b = 33,
	boomerang_guy_t = 34,
	fireball_guy_s = 35,
	fireball_guy_b_t = 36,
	fireball_guy_b = 37,
	fireball_guy_t = 38,
	hammerman_s = 39,
	hammerman_b_t = 40,
	hammerman_b = 41,
	hammerman_t = 42,
	orb = 48, //
	falling_brick = 50,
	jumping_brick = 51,
	ghost_free = 54,
	mad_flower = 56,
	mad_flower_walking = 57,
	bottom_flower = 59,
	jellyfish = 62,

};

enum Boss_compression_models {
	boss_1 = 10,
	boss_2 = 20,
	bowser = 30

};

/*
enum Enemy_compression_model_limits  {
        handler1_users_low = 1,
        handler1_users_high = 20,

        flowers_low=81,flowers_high=88,
        flower1_low=81,flower1_high=84,
        flower2_low=85,flower2_high=88,

        boomerang_guy_users_low=91,boomerang_guy_users_high=106,


        f_low =1,
        f_high=70,

        f_d0_e_d1_low=1,
        f_d0_e_d1_high=40,

        f_d0_low=41,
        f_d0_high=60,

        f_d0_d1_low=61,
        f_d0_d1_high=70,

        d0_d1_low=71,
        d0_d1_high=80,



};

enum Enemy_compression_models  {
        goomba_free=1,goomba_limited=2,
        turtle_free=3,turtle_limited=4,

        beetle_free=7,beetle_limited=8,
        fish_free=9,fish_limited=10,
        skel_tur_free=11,skel_tur_limited=12,

        flying_goomba = 21,
        flying_fish = 24,
        flying_turtle = 26,
        bomb = 28,

        horizontal_cannon = 42,
        diagonal_cannon = 44,

        lavaball = 62,

        ghost_limited = 72,

        flower1_upside_down = 83,flower1 = 84,
        flower2_upside_down = 87,flower2 = 88,

        boomerang_guy_s = 91,boomerang_guy_b_t = 92,boomerang_guy_b =
93,boomerang_guy_t = 94, fireball_guy_s = 95,fireball_guy_b_t =
96,fireball_guy_b = 97,fireball_guy_t = 98, hammerman_s = 99,hammerman_b_t =
100,hammerman_b = 101,hammerman_t = 102, orb = 107, falling_brick = 108,
        jumping_brick = 109,
        ghost_free = 110,
        mad_flower = 111,
        bottom_flower = 113,
        jellyfish = 114,



};
*/
