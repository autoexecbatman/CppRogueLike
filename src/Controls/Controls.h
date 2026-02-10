#pragma once

#include <curses.h>

//==CONTROLS==
// the enumeration for the controls of the player
enum class Controls
{
	// the controls for the player movement
	UP_ARROW = KEY_UP,
	DOWN_ARROW = KEY_DOWN,
	LEFT_ARROW = KEY_LEFT,
	RIGHT_ARROW = KEY_RIGHT,

	// Web-compatible WASD movement  
	W_KEY = 'w',
	S_KEY = 's',
	A_KEY = 'a', 
	D_KEY = 'd',

	// WASD diagonals
	Q_KEY = 'q',  // up-left
	E_KEY = 'e',  // up-right
	Z_KEY = 'z',  // down-left
	C_KEY = 'c',  // down-right

	MOUSE = KEY_MOUSE,

	TEST_COMMAND = 'x', // test command for debugging (changed from '{')

	WAIT = 'h',
	PICK = 'p',
	DROP = 'l',
	INVENTORY = 'i',
	ESCAPE = 27,
	CHAR_SHEET = '@',
	DESCEND = '>',
	TARGET = 't',
	TOGGLE_GRIP = 'T',
	QUIT = '~',
	DEBUG = 'b',
	ITEM_DISTRIBUTION = 'B',
	REVEAL = 'm',
	REGEN = 'n',
	OPEN_DOOR = 'o',
	CLOSE_DOOR = 'k',
	REST = 'r',
	HELP = '?',
	HIDE = 'H',
	CAST = 'C',
};
