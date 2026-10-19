#pragma once

#include "Vector2D.h"

class InputSystem;

class InputHandler
{
private:
	int keyPress{ -1 };
	int lastKey{ 0 };
	bool screenResized{ false };
	bool logicFrame{ true };
	Vector2D lastMousePos{ 0, 0 };
	Vector2D currentMousePos{ 0, 0 };

public:
	void key_store() noexcept;
	void key_listen(InputSystem& input) noexcept;

	bool mouse_moved() const noexcept;
	Vector2D get_mouse_position(InputSystem& input, int tile_size) noexcept;
	Vector2D get_mouse_position_old() const noexcept;

	int get_current_key() const noexcept { return keyPress; }

	int get_last_key() const noexcept { return lastKey; }

	void reset_key() noexcept
	{
		keyPress = -1;
		logicFrame = true;
	}

	// Whether this frame's input advances the game: true for any recognised key or a
	// resize, false when no key arrived and the frame only animates.
	bool is_logic_frame() const noexcept { return logicFrame; }

	bool was_resized() const noexcept { return screenResized; }

	void clear_resize() noexcept { screenResized = false; }
};
