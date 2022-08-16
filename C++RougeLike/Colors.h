#pragma once

//====
//define the color pairs for the game

constexpr auto DARK_WALL_PAIR = 1;
constexpr auto EMPTY_PAIR = 1;
constexpr auto DARK_GROUND_PAIR = 2;
constexpr auto MOUNTAIN_PAIR = 3;
constexpr auto ORC_PAIR = 4;
constexpr auto PLAYER_PAIR = 5;
constexpr auto WALL_PAIR = 6;
constexpr auto LIGHT_WALL_PAIR = 7;
constexpr auto LIGHT_GROUND_PAIR = 8;
constexpr auto DEAD_NPC_PAIR = 9;
constexpr auto TROLL_PAIR = 10;


struct Colors
{
	void my_init_pair();
};