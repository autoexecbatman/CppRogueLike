// file: Colors.h
#ifndef COLORS_H
#define COLORS_H

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
constexpr auto HPBARFULL_PAIR = 11;
constexpr auto HPBARMISSING_PAIR = 12;
constexpr auto LIGHTNING_PAIR = 13;
constexpr auto WHITE_PAIR = 14;
constexpr auto GOBLIN_PAIR = 15;
constexpr auto DRAGON_PAIR = 16;
constexpr auto FIREBALL_PAIR = 17;
constexpr auto CONFUSION_PAIR = 18;
constexpr auto WATER_PAIR = 19;
constexpr auto GOLD_PAIR = 20;


struct Colors
{
	void my_init_pair() noexcept;
};

#endif // !COLORS_H
// file: Colors.h
