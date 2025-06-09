#ifndef CONTROLS_H
#define CONTROLS_H

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

	// input for the player to wait
	WAIT = ' ',
	WAIT_ARROW_NUMPAD = KEY_B2,

	// input for the player to hit himself
	HIT_SELF = 'k',

	// Web-compatible WASD movement  
	W_KEY = 'w',
	S_KEY = 's',
	A_KEY = 'a', 
	D_KEY = 'd',

	// WASD diagonals
	Q_KEY = 'q',  // up-left
	E_KEY = 'e',  // up-right
	Z_KEY = 'z',  // down-left
	X_KEY = 'x',  // down-right

	// input for the player to pick items 
	PICK = 'p',
	PICK_SHIFT_STAR = '*',
	PICK_NUMPAD = PADSTAR,

	// input for displaying the inventory
	INVENTORY = 'i',

	// input for displaying the game menu
	ESCAPE = 27,

	MOUSE = KEY_MOUSE,

	HEAL = 'a',

	// input for the player to drop items
	DROP = 'd',

	CHAR_SHEET = '@',

	DESCEND = '>',

	// input for the player to target 
	TARGET = 't',

	// the controls for the player to exit the game
	QUIT = 'q',

	// the controls for the player to enter debug mode
	DEBUG = 'z',

	// the controls for the player to reveal the map
	REVEAL = 'm',

	// the controls for the player to regenerate the map
	REGEN = 'n',

	OPEN_DOOR = 'o',
	CLOSE_DOOR = 'c',

	REST = 'r',

	HELP = 'h',

};
#endif // !CONTROLS_H
