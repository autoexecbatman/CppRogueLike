#pragma once

//====
//define the color pairs for the game
#define GRASS_PAIR     1
#define EMPTY_PAIR     1
#define WATER_PAIR     2
#define MOUNTAIN_PAIR  3
#define ORC_PAIR       4
#define PLAYER_PAIR    5
#define WALL_PAIR      6
#define LIGHT_WALL_PAIR 7
#define LIGHT_GROUND_PAIR 8
#define DEAD_NPC_PAIR 9
#define TROLL_PAIR 10

class Colors
{public:
	void my_init_pair();
};