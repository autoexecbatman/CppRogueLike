#pragma once

//==CONTROLS==
// the enumeration for the controls of the player
enum class Controls
{
	// Cardinal movement (arrow keys + numpad 8/4/6/2)
	UP_ARROW = 0x103,
	DOWN_ARROW = 0x102,
	LEFT_ARROW = 0x104,
	RIGHT_ARROW = 0x105,

	// Diagonal movement (numpad 7/9/1/3)
	KP_NW = 0x110,
	KP_NE = 0x111,
	KP_SW = 0x112,
	KP_SE = 0x113,

	MOUSE = 0x199,
	MOUSE_RIGHT = 0x19A,

	TEST_COMMAND = 'x',

	WAIT = 0x106,
	PICK = 'p',
	DROP = 'd',
	INVENTORY = 'i',
	ESCAPE = 27,
	CHAR_SHEET = '@',
	DESCEND = '>',
	TARGET = 't',
	TOGGLE_GRIP = 'T',
	QUIT = '~',
	DEBUG = 'b',
	BALANCE_VIEWER = 'B',
	REVEAL = 'm',
	REGEN = 'n',
	OPEN_DOOR = 'o',
	CLOSE_DOOR = 'k',
	REST = 'r',
	HELP = '?',
	HIDE = 'H',
	CAST = 'C',
	USE = 'u',
};
