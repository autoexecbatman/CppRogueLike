#pragma once

#include <array>
#include <string_view>

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
	DISARM = 's',
	REST = 'r',
	HELP = '?',
	HIDE = 'H',
	CAST = 'C',
	USE = 'u',
	TURN_UNDEAD = 'U',
};


// What the help screen shows. Every command below is keyed by a character, and the
// enum value above is that character, so the key shown is read from the binding
// rather than typed out a second time. The list that used to live in DisplayManager
// was wrong on four of eleven lines - it offered comma for pick up, period for wait,
// s for spells and q for quit, none of which the game reads.
struct CommandHelp
{
	Controls control{};
	std::string_view description{};
};

inline constexpr std::array<CommandHelp, 15> CHARACTER_COMMANDS{ {
	{ Controls::PICK, "Pick up" },
	{ Controls::DROP, "Drop" },
	{ Controls::INVENTORY, "Inventory" },
	{ Controls::USE, "Use item" },
	{ Controls::CHAR_SHEET, "Character" },
	{ Controls::CAST, "Cast spell" },
	{ Controls::TARGET, "Fire missile" },
	{ Controls::TOGGLE_GRIP, "Two-hand grip" },
	{ Controls::REST, "Rest" },
	{ Controls::HIDE, "Hide" },
	{ Controls::TURN_UNDEAD, "Turn undead" },
	{ Controls::OPEN_DOOR, "Open door" },
	{ Controls::CLOSE_DOOR, "Close door" },
	{ Controls::DISARM, "Disarm trap" },
	{ Controls::DESCEND, "Descend stairs" },
} };

// The keys that are not characters, so their enum value says nothing a player could
// press. These are the only ones written out by hand.
struct NamedCommand
{
	std::string_view description{};
	std::string_view keys{};
};

inline constexpr std::array<NamedCommand, 4> NAMED_COMMANDS{ {
	{ "Movement", "arrows / numpad" },
	{ "Wait", "numpad 5 / h" },
	{ "Minimap", "Tab" },
	{ "Zoom", "+ / -" },
} };

// The character a command is triggered by, taken from its own binding.
[[nodiscard]] inline constexpr char command_key(Controls control)
{
	return static_cast<char>(static_cast<int>(control));
}
