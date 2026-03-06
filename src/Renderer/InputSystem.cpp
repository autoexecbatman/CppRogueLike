// file: InputSystem.cpp
#include <raylib.h>

#include "../Utils/Vector2D.h"
#include "InputSystem.h"

void InputSystem::poll()
{
	current_key = GameKey::NONE;
	char_input = 0;
	resized = false;

	if (IsWindowResized())
	{
		resized = true;
		current_key = GameKey::WINDOW_RESIZE;
		return;
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		current_key = GameKey::MOUSE_LEFT;
		return;
	}
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		current_key = GameKey::MOUSE_RIGHT;
		return;
	}

	// Controlled key-repeat: if a repeatable key is still held and the timer
	// has elapsed, fire it again without waiting for a new press event.
	if (held_raylib_key != 0)
	{
		if (IsKeyDown(held_raylib_key))
		{
			double now = GetTime();
			if (now - hold_start >= REPEAT_DELAY && now - last_repeat >= REPEAT_INTERVAL)
			{
				current_key = held_game_key;
				char_input = held_char_input;
				last_repeat = now;
				return;
			}
		}
		else
		{
			held_raylib_key = 0;
			held_game_key = GameKey::NONE;
			held_char_input = 0;
		}
	}

	bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

	auto register_key = [&](int raylib_key, GameKey gk, int ch, bool repeatable)
	{
		current_key = gk;
		char_input = ch;
		if (repeatable)
		{
			held_raylib_key = raylib_key;
			held_game_key = gk;
			held_char_input = ch;
			hold_start = GetTime();
			last_repeat = GetTime();
		}
		else
		{
			held_raylib_key = 0;
			held_game_key = GameKey::NONE;
			held_char_input = 0;
		}
	};

	// GetKeyPressed() reads from the GLFW key-press queue, which is populated
	// by the GLFW key callback on GLFW_PRESS. This avoids the IsKeyPressed
	// prev/curr timing dependency that causes input failure on Emscripten with
	// ASYNCIFY -- no matter when PollInputEvents runs, the queue retains events
	// until they are consumed here.
	int new_key = GetKeyPressed();

	switch (new_key)
	{
	case KEY_ENTER:
		if (alt)
		{
			ToggleFullscreen();
			return;
		}
		register_key(KEY_ENTER, GameKey::ENTER, 0, false);
		return;
	case KEY_ESCAPE:
		register_key(KEY_ESCAPE, GameKey::ESCAPE, 0, false);
		return;
	case KEY_TAB:
		register_key(KEY_TAB, GameKey::TAB, 0, false);
		return;
	case KEY_SPACE:
		register_key(KEY_SPACE, GameKey::SPACE, ' ', false);
		return;
	case KEY_BACKSPACE:
		register_key(KEY_BACKSPACE, GameKey::BACKSPACE, 0, false);
		return;

	// Arrow keys
	case KEY_UP:
		register_key(KEY_UP, GameKey::UP, 0, true);
		return;
	case KEY_DOWN:
		register_key(KEY_DOWN, GameKey::DOWN, 0, true);
		return;
	case KEY_LEFT:
		register_key(KEY_LEFT, GameKey::LEFT, 0, true);
		return;
	case KEY_RIGHT:
		register_key(KEY_RIGHT, GameKey::RIGHT, 0, true);
		return;

	// WASD + diagonals
	case KEY_W:
		register_key(KEY_W, GameKey::W, shift ? 'W' : 'w', true);
		return;
	case KEY_A:
		register_key(KEY_A, GameKey::A, shift ? 'A' : 'a', true);
		return;
	case KEY_S:
		if (ctrl)
		{
			register_key(KEY_S, GameKey::DECOR_SAVE, 0, false);
		}
		else
		{
			register_key(KEY_S, GameKey::S, shift ? 'S' : 's', true);
		}
		return;
	case KEY_D:
		register_key(KEY_D, GameKey::D, shift ? 'D' : 'd', true);
		return;
	case KEY_Q:
		register_key(KEY_Q, GameKey::Q, shift ? 'Q' : 'q', true);
		return;
	case KEY_E:
		register_key(KEY_E, GameKey::E, shift ? 'E' : 'e', true);
		return;
	case KEY_Z:
		register_key(KEY_Z, GameKey::Z, shift ? 'Z' : 'z', true);
		return;

	// Shift-ambiguous keys
	case KEY_C:
		if (shift)
		{
			register_key(KEY_C, GameKey::CAST, 'C', false);
		}
		else
		{
			register_key(KEY_C, GameKey::C, 'c', true);
		}
		return;
	case KEY_H:
		if (shift)
		{
			register_key(KEY_H, GameKey::HIDE, 'H', false);
		}
		else
		{
			register_key(KEY_H, GameKey::WAIT, 'h', true);
		}
		return;
	case KEY_T:
		if (shift)
		{
			register_key(KEY_T, GameKey::TOGGLE_GRIP, 'T', false);
		}
		else
		{
			register_key(KEY_T, GameKey::TARGET, 't', false);
		}
		return;
	case KEY_B:
		if (shift)
		{
			register_key(KEY_B, GameKey::ITEM_DISTRIBUTION, 'B', false);
		}
		else
		{
			register_key(KEY_B, GameKey::DEBUG, 'b', false);
		}
		return;

	// Action keys
	case KEY_P:
		register_key(KEY_P, GameKey::PICK, shift ? 'P' : 'p', false);
		return;
	case KEY_L:
		register_key(KEY_L, GameKey::DROP, shift ? 'L' : 'l', false);
		return;
	case KEY_I:
		register_key(KEY_I, GameKey::INVENTORY, shift ? 'I' : 'i', false);
		return;
	case KEY_O:
		register_key(KEY_O, GameKey::OPEN_DOOR, shift ? 'O' : 'o', false);
		return;
	case KEY_K:
		register_key(KEY_K, GameKey::CLOSE_DOOR, shift ? 'K' : 'k', false);
		return;
	case KEY_R:
		register_key(KEY_R, GameKey::REST, shift ? 'R' : 'r', false);
		return;
	case KEY_U:
		register_key(KEY_U, GameKey::USE, shift ? 'U' : 'u', false);
		return;
	case KEY_X:
		register_key(KEY_X, GameKey::TEST_COMMAND, shift ? 'X' : 'x', false);
		return;
	case KEY_M:
		register_key(KEY_M, GameKey::REVEAL, shift ? 'M' : 'm', false);
		return;
	case KEY_N:
		register_key(KEY_N, GameKey::REGEN, shift ? 'N' : 'n', false);
		return;

	// Zoom
	case KEY_EQUAL:
		register_key(KEY_EQUAL, GameKey::ZOOM_IN, '=', false);
		return;
	case KEY_MINUS:
		register_key(KEY_MINUS, GameKey::ZOOM_OUT, '-', false);
		return;

	// Editor keys
	case KEY_F2:
		register_key(KEY_F2, GameKey::DECOR_EDIT_TOGGLE, 0, false);
		return;
	case KEY_F3:
		register_key(KEY_F3, GameKey::CONTENT_EDIT_TOGGLE, 0, false);
		return;
	case KEY_COMMA:
		register_key(KEY_COMMA, GameKey::DECOR_PREV, 0, false);
		return;
	case KEY_PERIOD:
		register_key(KEY_PERIOD, GameKey::DECOR_NEXT, 0, false);
		return;

	default:
		break;
	}

	// Shift+symbol keys and char_input for text fields
	int ch = GetCharPressed();
	if (ch > 0)
	{
		char_input = ch;
		switch (ch)
		{
		case '@':
			current_key = GameKey::CHAR_SHEET;
			return;
		case '>':
			current_key = GameKey::DESCEND;
			return;
		case '?':
			current_key = GameKey::HELP;
			return;
		case '~':
			current_key = GameKey::QUIT;
			return;
		default:
			break;
		}
	}
}

Vector2D InputSystem::get_mouse_tile(int tile_size) const
{
	if (tile_size <= 0)
		return Vector2D{ 0, 0 };

	::Vector2 mouse_pos = GetMousePosition();
	return Vector2D{
		static_cast<int>(mouse_pos.x) / tile_size,
		static_cast<int>(mouse_pos.y) / tile_size
	};
}

Vector2D InputSystem::get_mouse_world_tile(int cam_x, int cam_y, int tile_size) const
{
	if (tile_size <= 0)
		return Vector2D{ 0, 0 };

	::Vector2 mouse_pos = GetMousePosition();
	return Vector2D{
		(static_cast<int>(mouse_pos.x) + cam_x) / tile_size,
		(static_cast<int>(mouse_pos.y) + cam_y) / tile_size
	};
}

// end of file: InputSystem.cpp
