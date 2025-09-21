// file: Colors.h
#ifndef COLORS_H
#define COLORS_H

// Color pairs organized by foreground_background pattern
// Grouped logically for easy editing

// === WHITE FOREGROUND PAIRS ===
inline constexpr auto WHITE_BLACK_PAIR = 1;
inline constexpr auto WHITE_RED_PAIR = 2;
inline constexpr auto WHITE_BLUE_PAIR = 3;
inline constexpr auto WHITE_GREEN_PAIR = 4;

// === BLACK FOREGROUND PAIRS ===
inline constexpr auto BLACK_WHITE_PAIR = 5;
inline constexpr auto BLACK_GREEN_PAIR = 6;
inline constexpr auto BLACK_YELLOW_PAIR = 7;
inline constexpr auto BLACK_RED_PAIR = 8;

// === COLORED FOREGROUND ON BLACK ===
inline constexpr auto RED_BLACK_PAIR = 9;
inline constexpr auto GREEN_BLACK_PAIR = 10;
inline constexpr auto YELLOW_BLACK_PAIR = 11;
inline constexpr auto BLUE_BLACK_PAIR = 12;
inline constexpr auto CYAN_BLACK_PAIR = 13;
inline constexpr auto MAGENTA_BLACK_PAIR = 14;

// === SPECIAL COMBINATIONS ===
inline constexpr auto CYAN_BLUE_PAIR = 15;
inline constexpr auto RED_WHITE_PAIR = 16;
inline constexpr auto GREEN_YELLOW_PAIR = 17;
inline constexpr auto GREEN_MAGENTA_PAIR = 18;
inline constexpr auto RED_YELLOW_PAIR = 19;
inline constexpr auto GREEN_RED_PAIR = 20;

// === CUSTOM COLORS ===
inline constexpr auto BROWN_BLACK_PAIR = 21;
inline constexpr auto DIM_GREEN_BLACK_PAIR = 22;

struct Colors
{
	void my_init_pair() noexcept;
};

#endif // !COLORS_H
// file: Colors.h
