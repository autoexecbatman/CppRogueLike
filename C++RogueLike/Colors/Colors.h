// file: Colors.h
#ifndef COLORS_H
#define COLORS_H

// define the color pairs for the game
inline constexpr auto DARK_WALL_PAIR = 1;
inline constexpr auto EMPTY_PAIR = 1;
inline constexpr auto DARK_GROUND_PAIR = 2;
inline constexpr auto MOUNTAIN_PAIR = 3;
inline constexpr auto ORC_PAIR = 4;
inline constexpr auto PLAYER_PAIR = 5;
inline constexpr auto WALL_PAIR = 6;
inline constexpr auto LIGHT_WALL_PAIR = 7;
inline constexpr auto LIGHT_GROUND_PAIR = 8;
inline constexpr auto DEAD_NPC_PAIR = 9;
inline constexpr auto TROLL_PAIR = 10;
inline constexpr auto HPBARFULL_PAIR = 11;
inline constexpr auto HPBARMISSING_PAIR = 12;
inline constexpr auto LIGHTNING_PAIR = 13;
inline constexpr auto WHITE_PAIR = 14;
inline constexpr auto GOBLIN_PAIR = 15;
inline constexpr auto DRAGON_PAIR = 16;
inline constexpr auto FIREBALL_PAIR = 17;
inline constexpr auto CONFUSION_PAIR = 18;
inline constexpr auto WATER_PAIR = 19;
inline constexpr auto GOLD_PAIR = 20;
inline constexpr auto DOOR_PAIR = 21;

struct Colors
{
	void my_init_pair() noexcept;
};

#endif // !COLORS_H
// file: Colors.h
