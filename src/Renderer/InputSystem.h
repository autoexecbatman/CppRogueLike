#pragma once

#include "../Utils/Vector2D.h"

enum class GameKey
{
	NONE = 0,

	// Movement
	UP,
	DOWN,
	LEFT,
	RIGHT,

	// WASD movement
	W,
	A,
	S,
	D,

	// Diagonals
	Q,
	E,
	Z,
	C,

	// Actions
	ENTER,
	ESCAPE,
	TAB,
	SPACE,
	BACKSPACE,

	// Game actions
	PICK,
	DROP,
	INVENTORY,
	CHAR_SHEET,
	DESCEND,
	TARGET,
	TOGGLE_GRIP,
	WAIT,
	OPEN_DOOR,
	CLOSE_DOOR,
	REST,
	HELP,
	HIDE,
	CAST,
	USE,
	QUIT,

	// Debug
	DEBUG,
	TEST_COMMAND,
	ITEM_DISTRIBUTION,
	REVEAL,
	REGEN,

	// Mouse
	MOUSE_LEFT,
	MOUSE_RIGHT,

	// Window
	WINDOW_RESIZE,

	// Zoom
	ZOOM_IN,
	ZOOM_OUT,

	// Decor editor
	DECOR_EDIT_TOGGLE,
	DECOR_PREV,
	DECOR_NEXT,
	DECOR_SAVE,
	DECOR_BROWSER,

	// Content editor (developer tool -- F3)
	CONTENT_EDIT_TOGGLE,

	// Minimap overlay
	MINIMAP_TOGGLE
};

class InputSystem
{
private:
	GameKey currentKey{ GameKey::NONE };
	int charInput{ 0 };
	bool resized{ false };

	// Controlled key-repeat state
	// Repeatable keys (movement + wait) fire once on press, then again after
	// REPEAT_DELAY, and every REPEAT_INTERVAL thereafter while held.
	int heldRaylibKey{ 0 };
	GameKey heldGameKey{ GameKey::NONE };
	int heldCharInput{ 0 };
	double holdStart{ 0.0 };
	double lastRepeat{ 0.0 };

public:
	InputSystem() = default;
	~InputSystem() = default;

	InputSystem(const InputSystem&) = delete;
	InputSystem& operator=(const InputSystem&) = delete;
	InputSystem(InputSystem&&) = delete;
	InputSystem& operator=(InputSystem&&) = delete;

	void poll();

	[[nodiscard]] GameKey get_key() const { return currentKey; }
	[[nodiscard]] int get_char_input() const { return charInput; }
	[[nodiscard]] Vector2D get_mouse_tile(int tileSize) const;
	[[nodiscard]] Vector2D get_mouse_world_tile(int camX, int camY, int tileSize) const;
	[[nodiscard]] bool has_player_action() const { return currentKey != GameKey::NONE; }
	[[nodiscard]] bool window_resized() const { return resized; }

};
