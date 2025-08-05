// file: Colors.cpp
#include <iostream>
#include <curses.h>
#include "Colors.h"

//==COLORS==
// initializes all the color pairs
void Colors::my_init_pair() noexcept
{
	// Color pairs organized by foreground_background pattern
	// Grouped logically for easy editing
	
	// === WHITE FOREGROUND PAIRS ===
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_RED);
	init_pair(3, COLOR_WHITE, COLOR_BLUE);
	// Create custom dim green for background to reduce brightness
	init_color(101, 0, 400, 0); // Dim green - same as slot 9 but in slot 101
	init_pair(4, COLOR_WHITE, 101); // Use dim green background instead of bright COLOR_GREEN
	
	// === BLACK FOREGROUND PAIRS ===
	init_pair(5, COLOR_BLACK, COLOR_WHITE);
	init_pair(6, COLOR_BLACK, COLOR_GREEN);
	init_pair(7, COLOR_BLACK, COLOR_YELLOW);
	init_pair(8, COLOR_BLACK, COLOR_RED);
	
	// === COLORED FOREGROUND ON BLACK ===
	init_pair(9, COLOR_RED, COLOR_BLACK);
	init_pair(10, COLOR_GREEN, COLOR_BLACK);
	init_pair(11, COLOR_YELLOW, COLOR_BLACK);
	init_pair(12, COLOR_BLUE, COLOR_BLACK);
	init_pair(13, COLOR_CYAN, COLOR_BLACK);
	
	// === SPECIAL COMBINATIONS ===
	init_pair(14, COLOR_CYAN, COLOR_BLUE);
	init_pair(15, COLOR_RED, COLOR_WHITE);
	init_pair(16, COLOR_GREEN, COLOR_YELLOW);
	init_pair(17, COLOR_GREEN, COLOR_MAGENTA);
	init_pair(18, COLOR_RED, COLOR_YELLOW);
	init_pair(19, COLOR_GREEN, COLOR_RED);
	
	// === CUSTOM COLORS ===
	// Create custom brown color
	init_color(100, 500, 300, 0); // Use slot 100 to avoid conflicts
	init_pair(20, 100, COLOR_BLACK);
	
	// Create custom dim green color
	init_color(9, 0, 400, 0);
	init_pair(21, 9, COLOR_BLACK);
}

// end of file: Colors.cpp
