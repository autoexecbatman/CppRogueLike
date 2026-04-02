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

#ifdef EMSCRIPTEN
	// On Emscripten, the GLFW key-press queue (GetKeyPressed) is never
	// populated because the GLFW key callback does not fire on the web
	// platform. IsKeyPressed (prev/curr state transition) works correctly
	// because PollInputEvents() is called explicitly in end_frame() before
	// SwapScreenBuffer(), keeping prev/curr in sync.
	auto is_pressed = [](int k) { return IsKeyPressed(k); };
#else
	// On native, prefer the GLFW queue -- it is not subject to prev/curr
	// timing issues and handles rapid key-press/release within one frame.
	int queued = GetKeyPressed();
	auto is_pressed = [queued](int k) { return queued == k; };
#endif
	int new_key = 0;
	for (int k : {
		KEY_ENTER, KEY_ESCAPE, KEY_TAB, KEY_SPACE, KEY_BACKSPACE,
		KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
		KEY_W, KEY_A, KEY_S, KEY_D, KEY_Q, KEY_E, KEY_Z, KEY_C,
		KEY_H, KEY_T, KEY_B, KEY_P, KEY_L, KEY_I, KEY_O, KEY_K,
		KEY_R, KEY_U, KEY_X, KEY_M, KEY_N,
		KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
		KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO,
		KEY_F2, KEY_F3, KEY_EQUAL, KEY_MINUS, KEY_COMMA, KEY_PERIOD,
		KEY_SLASH, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE })
	{
		if (is_pressed(k)) { new_key = k; break; }
	}

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
		register_key(KEY_TAB, GameKey::MINIMAP_TOGGLE, 0, false);
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
#ifndef EMSCRIPTEN
	case KEY_F2:
		register_key(KEY_F2, GameKey::DECOR_EDIT_TOGGLE, 0, false);
		return;
	case KEY_F3:
		register_key(KEY_F3, GameKey::CONTENT_EDIT_TOGGLE, 0, false);
		return;
#endif
	case KEY_COMMA:
		if (!shift)
		{
			register_key(KEY_COMMA, GameKey::DECOR_PREV, 0, false);
			return;
		}
		break;
	case KEY_PERIOD:
		if (!shift)
		{
			register_key(KEY_PERIOD, GameKey::DECOR_NEXT, 0, false);
			return;
		}
		break;

	default:
		break;
	}

	// Shift+symbol keys and char_input for text fields.
	// GetCharPressed() is also broken on Emscripten (queue never filled),
	// so derive char from the pressed key + shift state on web.
#ifdef EMSCRIPTEN
	int ch = 0;
	if (new_key >= KEY_A && new_key <= KEY_Z)
	{
		ch = shift ? (new_key - KEY_A + 'A') : (new_key - KEY_A + 'a');
	}
	else if (new_key == KEY_ONE)   { ch = shift ? '!' : '1'; }
	else if (new_key == KEY_TWO)   { ch = shift ? '@' : '2'; }
	else if (new_key == KEY_THREE) { ch = shift ? '#' : '3'; }
	else if (new_key == KEY_FOUR)  { ch = shift ? '$' : '4'; }
	else if (new_key == KEY_FIVE)  { ch = shift ? '%' : '5'; }
	else if (new_key == KEY_SIX)   { ch = shift ? '^' : '6'; }
	else if (new_key == KEY_SEVEN) { ch = shift ? '&' : '7'; }
	else if (new_key == KEY_EIGHT) { ch = shift ? '*' : '8'; }
	else if (new_key == KEY_NINE)  { ch = shift ? '(' : '9'; }
	else if (new_key == KEY_ZERO)  { ch = shift ? ')' : '0'; }
	else if (new_key == KEY_EQUAL) { ch = shift ? '+' : '='; }
	else if (new_key == KEY_MINUS) { ch = shift ? '_' : '-'; }
	else if (new_key == KEY_COMMA) { ch = shift ? '<' : ','; }
	else if (new_key == KEY_PERIOD) { ch = shift ? '>' : '.'; }
	else if (new_key == KEY_SLASH) { ch = shift ? '?' : '/'; }
	else if (new_key == KEY_SEMICOLON) { ch = shift ? ':' : ';'; }
	else if (new_key == KEY_APOSTROPHE) { ch = shift ? '"' : '\''; }
	else if (new_key == KEY_GRAVE) { ch = shift ? '~' : '`'; }
	else if (new_key == KEY_SPACE) { ch = ' '; }
#else
	int ch = GetCharPressed();
#endif
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
