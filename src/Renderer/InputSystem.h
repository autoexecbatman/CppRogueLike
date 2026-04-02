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
public:
	InputSystem() = default;
	~InputSystem() = default;

	InputSystem(const InputSystem&) = delete;
	InputSystem& operator=(const InputSystem&) = delete;
	InputSystem(InputSystem&&) = delete;
	InputSystem& operator=(InputSystem&&) = delete;

	void poll();

	[[nodiscard]] GameKey get_key() const { return current_key; }
	[[nodiscard]] int get_char_input() const { return char_input; }
	[[nodiscard]] Vector2D get_mouse_tile(int tile_size) const;
	[[nodiscard]] Vector2D get_mouse_world_tile(int cam_x, int cam_y, int tile_size) const;
	[[nodiscard]] bool has_player_action() const { return current_key != GameKey::NONE; }
	[[nodiscard]] bool window_resized() const { return resized; }

private:
	GameKey current_key{ GameKey::NONE };
	int char_input{ 0 };
	bool resized{ false };

	// Controlled key-repeat state
	// Repeatable keys (movement + wait) fire once on press, then again after
	// REPEAT_DELAY, and every REPEAT_INTERVAL thereafter while held.
	int held_raylib_key{ 0 };
	GameKey held_game_key{ GameKey::NONE };
	int held_char_input{ 0 };
	double hold_start{ 0.0 };
	double last_repeat{ 0.0 };

	static constexpr double REPEAT_DELAY = 0.30; // seconds before repeat begins
	static constexpr double REPEAT_INTERVAL = 0.10; // seconds between repeat fires
};
