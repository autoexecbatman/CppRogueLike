#ifndef CONTROLS_H
#define CONTROLS_H

#include <curses.h>

//==CONTROLS==
// the enumeration for the controls of the player
enum class Controls
{
	// the controls for the player movement
	UP = '8',
	UP_ARROW = KEY_UP,
	UP_ARROW_NUMPAD = KEY_A2,

	DOWN = '2',
	DOWN_ARROW = KEY_DOWN,
	DOWN_ARROW_NUMPAD = KEY_C2,

	LEFT = '4',
	LEFT_ARROW = KEY_LEFT,
	LEFT_ARROW_NUMPAD = KEY_B1,

	RIGHT = '6',
	RIGHT_ARROW = KEY_RIGHT,
	RIGHT_ARROW_NUMPAD = KEY_B3,

	UP_LEFT = '7',
	UP_LEFT_ARROW_NUMPAD = KEY_A1,

	UP_RIGHT = '9',
	UP_RIGHT_ARROW_NUMPAD = KEY_A3,

	DOWN_LEFT = '1',
	DOWN_LEFT_ARROW_NUMPAD = KEY_C1,

	DOWN_RIGHT = '3',
	DOWN_RIGHT_ARROW_NUMPAD = KEY_C3,

	// input for the player to wait
	WAIT = '5',
	WAIT_ARROW_NUMPAD = KEY_B2,

	// input for the player to hit himself
	HIT_SELF = ' ',

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

	CHAR_SHEET = 'c',

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
	REGEN = 'r',
};
#endif // !CONTROLS_H
