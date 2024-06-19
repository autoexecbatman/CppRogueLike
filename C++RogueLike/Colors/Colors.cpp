// file: Colors.cpp
#include <iostream>
#include <curses.h>
#include "Colors.h"

//==COLORS==
// initializes all the color pairs
void Colors::my_init_pair() noexcept
{
	/*std::cout << "colors on" << std::endl;*/
	//====
	//the init_pair functions to make a color using curses
	//the first parameter is the color pair number
	//the second parameter is the foreground color
	//the third parameter is the background color
	//the init_pair functions must be called before any other curses functions
	init_pair(1, COLOR_WHITE, COLOR_BLACK);//grass color
	init_pair(2, COLOR_CYAN, COLOR_BLUE);//water color
	init_pair(3, COLOR_BLACK, COLOR_WHITE);//mountain color
	init_pair(4, COLOR_RED, COLOR_BLACK);//orc color
	init_pair(5, COLOR_GREEN, COLOR_MAGENTA);//player color
	init_pair(6, COLOR_WHITE, COLOR_BLACK);//wall color
	init_pair(7, COLOR_RED, COLOR_WHITE);//light wall color
	init_pair(8, COLOR_GREEN, COLOR_YELLOW);//light ground color
	init_pair(9, COLOR_RED, COLOR_BLACK);//dead npc color
	init_pair(10, COLOR_GREEN, COLOR_BLACK);//troll color
	init_pair(11, COLOR_WHITE, COLOR_GREEN);//hp full color
	init_pair(12, COLOR_WHITE, COLOR_RED);//hp missing color
	init_pair(13, COLOR_WHITE, COLOR_BLUE);//lightning color
	init_pair(14, COLOR_WHITE, COLOR_BLACK); // white color
	init_pair(15, COLOR_YELLOW, COLOR_BLACK); // goblin color
	init_pair(16, COLOR_GREEN, COLOR_RED); // dragon color
	init_pair(17, COLOR_RED, COLOR_YELLOW); // fireball color
	init_pair(18, COLOR_WHITE, COLOR_GREEN); // confusion color
	init_pair(19, COLOR_BLUE, COLOR_BLACK); // water color
	init_pair(20, COLOR_YELLOW, COLOR_BLACK); // gold color
	// make brown
	init_color(8, 500, 300, 0);
	init_pair(21, 8, COLOR_BLACK); // brown color
}

// end of file: Colors.cpp
