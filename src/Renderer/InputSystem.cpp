// file: InputSystem.cpp
#include <raylib.h>

#include "../Utils/Vector2D.h"
#include "InputSystem.h"

constexpr double REPEAT_DELAY = 0.30; // seconds before repeat begins
constexpr double REPEAT_INTERVAL = 0.10; // seconds between repeat fires

void InputSystem::poll()
{
	currentKey = GameKey::NONE;
	charInput = 0;
	resized = false;

	if (IsWindowResized())
	{
		resized = true;
		currentKey = GameKey::WINDOW_RESIZE;
		return;
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		currentKey = GameKey::MOUSE_LEFT;
		return;
	}
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		currentKey = GameKey::MOUSE_RIGHT;
		return;
	}

	// Controlled key-repeat: if a repeatable key is still held and the timer
	// has elapsed, fire it again without waiting for a new press event.
	if (heldRaylibKey != 0)
	{
		if (IsKeyDown(heldRaylibKey))
		{
			double now = GetTime();
			if (now - holdStart >= REPEAT_DELAY && now - lastRepeat >= REPEAT_INTERVAL)
			{
				currentKey = heldGameKey;
				charInput = heldCharInput;
				lastRepeat = now;
				return;
			}
		}
		else
		{
			heldRaylibKey = 0;
			heldGameKey = GameKey::NONE;
			heldCharInput = 0;
		}
	}

	bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

	auto register_key = [&](int raylibKey, GameKey gk, int ch, bool repeatable)
	{
		currentKey = gk;
		charInput = ch;
		if (repeatable)
		{
			heldRaylibKey = raylibKey;
			heldGameKey = gk;
			heldCharInput = ch;
			holdStart = GetTime();
			lastRepeat = GetTime();
		}
		else
		{
			heldRaylibKey = 0;
			heldGameKey = GameKey::NONE;
			heldCharInput = 0;
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
	auto isPressed = [queued](int k) { return queued == k; };
#endif
	int newKey = 0;
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
		if (isPressed(k)) { newKey = k; break; }
	}

	switch (newKey)
	{

	case KEY_ENTER:
	{
		if (alt)
		{
			ToggleFullscreen();
			return;
		}
		register_key(KEY_ENTER, GameKey::ENTER, 0, false);
		return;
	}

	case KEY_ESCAPE:
	{
		register_key(KEY_ESCAPE, GameKey::ESCAPE, 0, false);
		return;
	}

	case KEY_TAB:
	{
		register_key(KEY_TAB, GameKey::MINIMAP_TOGGLE, 0, false);
		return;
	}

	case KEY_SPACE:
	{
		register_key(KEY_SPACE, GameKey::SPACE, ' ', false);
		return;
	}

	case KEY_BACKSPACE:
	{
		register_key(KEY_BACKSPACE, GameKey::BACKSPACE, 0, false);
		return;
	}

	// Arrow keys
	case KEY_UP:
	{
		register_key(KEY_UP, GameKey::UP, 0, true);
		return;
	}

	case KEY_DOWN:
	{
		register_key(KEY_DOWN, GameKey::DOWN, 0, true);
		return;
	}

	case KEY_LEFT:
	{
		register_key(KEY_LEFT, GameKey::LEFT, 0, true);
		return;
	}

	case KEY_RIGHT:
	{
		register_key(KEY_RIGHT, GameKey::RIGHT, 0, true);
		return;
	}

	// WASD + diagonals
	case KEY_W:
	{
		register_key(KEY_W, GameKey::W, shift ? 'W' : 'w', true);
		return;
	}

	case KEY_A:
	{
		register_key(KEY_A, GameKey::A, shift ? 'A' : 'a', true);
		return;
	}

	case KEY_S:
	{
		if (ctrl)
		{
			register_key(KEY_S, GameKey::DECOR_SAVE, 0, false);
		}
		else
		{
			register_key(KEY_S, GameKey::S, shift ? 'S' : 's', true);
		}
		return;
	}

	case KEY_D:
	{
		register_key(KEY_D, GameKey::D, shift ? 'D' : 'd', true);
		return;
	}

	case KEY_Q:
	{
		register_key(KEY_Q, GameKey::Q, shift ? 'Q' : 'q', true);
		return;
	}

	case KEY_E:
	{
		register_key(KEY_E, GameKey::E, shift ? 'E' : 'e', true);
		return;
	}

	case KEY_Z:
	{
		register_key(KEY_Z, GameKey::Z, shift ? 'Z' : 'z', true);
		return;
	}

	// Shift-ambiguous keys
	case KEY_C:
	{
		if (shift)
		{
			register_key(KEY_C, GameKey::CAST, 'C', false);
		}
		else
		{
			register_key(KEY_C, GameKey::C, 'c', true);
		}
		return;
	}

	case KEY_H:
	{
		if (shift)
		{
			register_key(KEY_H, GameKey::HIDE, 'H', false);
		}
		else
		{
			register_key(KEY_H, GameKey::WAIT, 'h', true);
		}
		return;
	}

	case KEY_T:
	{
		if (shift)
		{
			register_key(KEY_T, GameKey::TOGGLE_GRIP, 'T', false);
		}
		else
		{
			register_key(KEY_T, GameKey::TARGET, 't', false);
		}
		return;
	}

	case KEY_B:
	{
		if (shift)
		{
			register_key(KEY_B, GameKey::ITEM_DISTRIBUTION, 'B', false);
		}
		else
		{
			register_key(KEY_B, GameKey::DEBUG, 'b', false);
		}
		return;
	}

	// Action keys
	case KEY_P:
	{
		register_key(KEY_P, GameKey::PICK, shift ? 'P' : 'p', false);
		return;
	}

	case KEY_L:
	{
		register_key(KEY_L, GameKey::DROP, shift ? 'L' : 'l', false);
		return;
	}

	case KEY_I:
	{
		register_key(KEY_I, GameKey::INVENTORY, shift ? 'I' : 'i', false);
		return;
	}

	case KEY_O:
	{
		register_key(KEY_O, GameKey::OPEN_DOOR, shift ? 'O' : 'o', false);
		return;
	}

	case KEY_K:
	{
		register_key(KEY_K, GameKey::CLOSE_DOOR, shift ? 'K' : 'k', false);
		return;
	}

	case KEY_R:
	{
		register_key(KEY_R, GameKey::REST, shift ? 'R' : 'r', false);
		return;
	}

	case KEY_U:
	{
		register_key(KEY_U, GameKey::USE, shift ? 'U' : 'u', false);
		return;
	}

	case KEY_X:
	{
		register_key(KEY_X, GameKey::TEST_COMMAND, shift ? 'X' : 'x', false);
		return;
	}

	case KEY_M:
	{
		register_key(KEY_M, GameKey::REVEAL, shift ? 'M' : 'm', false);
		return;
	}

	case KEY_N:
	{
		register_key(KEY_N, GameKey::REGEN, shift ? 'N' : 'n', false);
		return;
	}

	// Zoom
	case KEY_EQUAL:
	{
		register_key(KEY_EQUAL, GameKey::ZOOM_IN, '=', false);
		return;
	}

	case KEY_MINUS:
	{
		register_key(KEY_MINUS, GameKey::ZOOM_OUT, '-', false);
		return;
	}

	// Editor keys
#ifndef EMSCRIPTEN
	case KEY_F2:
	{
		register_key(KEY_F2, GameKey::DECOR_EDIT_TOGGLE, 0, false);
		return;
	}

	case KEY_F3:
	{
		register_key(KEY_F3, GameKey::CONTENT_EDIT_TOGGLE, 0, false);
		return;
	}
#endif

	case KEY_COMMA:
	{
		if (!shift)
		{
			register_key(KEY_COMMA, GameKey::DECOR_PREV, 0, false);
			return;
		}
		break;
	}

	case KEY_PERIOD:
	{
		if (!shift)
		{
			register_key(KEY_PERIOD, GameKey::DECOR_NEXT, 0, false);
			return;
		}
		break;
	}

	default:
	{
		break;
	}

	}

	// Shift+symbol keys and char_input for text fields.
	// GetCharPressed() is also broken on Emscripten (queue never filled),
	// so derive char from the pressed key + shift state on web.
#ifdef EMSCRIPTEN
	int ch = 0;
	if (newKey >= KEY_A && newKey <= KEY_Z)
	{
		ch = shift ? (newKey - KEY_A + 'A') : (newKey - KEY_A + 'a');
	}
	else if (newKey == KEY_ONE)   { ch = shift ? '!' : '1'; }
	else if (newKey == KEY_TWO)   { ch = shift ? '@' : '2'; }
	else if (newKey == KEY_THREE) { ch = shift ? '#' : '3'; }
	else if (newKey == KEY_FOUR)  { ch = shift ? '$' : '4'; }
	else if (newKey == KEY_FIVE)  { ch = shift ? '%' : '5'; }
	else if (newKey == KEY_SIX)   { ch = shift ? '^' : '6'; }
	else if (newKey == KEY_SEVEN) { ch = shift ? '&' : '7'; }
	else if (newKey == KEY_EIGHT) { ch = shift ? '*' : '8'; }
	else if (newKey == KEY_NINE)  { ch = shift ? '(' : '9'; }
	else if (newKey == KEY_ZERO)  { ch = shift ? ')' : '0'; }
	else if (newKey == KEY_EQUAL) { ch = shift ? '+' : '='; }
	else if (newKey == KEY_MINUS) { ch = shift ? '_' : '-'; }
	else if (newKey == KEY_COMMA) { ch = shift ? '<' : ','; }
	else if (newKey == KEY_PERIOD) { ch = shift ? '>' : '.'; }
	else if (newKey == KEY_SLASH) { ch = shift ? '?' : '/'; }
	else if (newKey == KEY_SEMICOLON) { ch = shift ? ':' : ';'; }
	else if (newKey == KEY_APOSTROPHE) { ch = shift ? '"' : '\''; }
	else if (newKey == KEY_GRAVE) { ch = shift ? '~' : '`'; }
	else if (newKey == KEY_SPACE) { ch = ' '; }
#else
	int ch = GetCharPressed();
#endif
	if (ch > 0)
	{
		charInput = ch;
		switch (ch)
		{

		case '@':
		{
			currentKey = GameKey::CHAR_SHEET;
			return;
		}

		case '>':
		{
			currentKey = GameKey::DESCEND;
			return;
		}

		case '?':
		{
			currentKey = GameKey::HELP;
			return;
		}

		case '~':
		{
			currentKey = GameKey::QUIT;
			return;
		}

		default:
		{
			break;
		}

		}
	}
}

Vector2D InputSystem::get_mouse_tile(int tileSize) const
{
	if (tileSize <= 0)
	{
		return Vector2D{ 0, 0 };
	}

	::Vector2 mousePos = GetMousePosition();
	return Vector2D{
		static_cast<int>(mousePos.x) / tileSize,
		static_cast<int>(mousePos.y) / tileSize
	};
}

Vector2D InputSystem::get_mouse_world_tile(int camX, int camY, int tileSize) const
{
	if (tileSize <= 0)
	{
		return Vector2D{ 0, 0 };
	}

	::Vector2 mouse_pos = GetMousePosition();
	return Vector2D{
		(static_cast<int>(mouse_pos.x) + camX) / tileSize,
		(static_cast<int>(mouse_pos.y) + camY) / tileSize
	};
}

// end of file: InputSystem.cpp
