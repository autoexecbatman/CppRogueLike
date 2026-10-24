// file: EditorInput.cpp
#include <array>

#include <raylib.h>

#include "EditorInput.h"
#include "Renderer.h"

namespace
{
// One raylib key and the editor key it stands for.
struct KeyBinding
{
	int raylibKey{ 0 };
	EditorKey editorKey{ EditorKey::ESCAPE };
};

// Every key the editors read as pressed, in one list.
constexpr std::array<KeyBinding, 12> PRESSED_KEYS{ {
	{ KEY_ESCAPE, EditorKey::ESCAPE },
	{ KEY_TAB, EditorKey::TAB },
	{ KEY_ENTER, EditorKey::ENTER },
	{ KEY_UP, EditorKey::UP },
	{ KEY_DOWN, EditorKey::DOWN },
	{ KEY_LEFT, EditorKey::LEFT },
	{ KEY_RIGHT, EditorKey::RIGHT },
	{ KEY_F2, EditorKey::PICKER },
	{ KEY_A, EditorKey::ADD },
	{ KEY_S, EditorKey::SAVE },
	{ KEY_DELETE, EditorKey::REMOVE },
	{ KEY_BACKSPACE, EditorKey::BACKSPACE },
} };
} // namespace

EditorInput read_editor_input(const Renderer& renderer)
{
	EditorInput input{};

	// Each key that went down this frame.
	for (const KeyBinding& binding : PRESSED_KEYS)
	{
		if (IsKeyPressed(binding.raylibKey))
		{
			input.pressed.insert(binding.editorKey);
		}
	}
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		input.pressed.insert(EditorKey::CLICK);
	}

	// Control is read held, as a modifier.
	if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
	{
		input.held.insert(EditorKey::CONTROL);
	}

	// The mouse, truncated to whole pixels as every reader used it.
	const ::Vector2 mouse = GetMousePosition();
	input.mouse =
		Vector2D{ static_cast<int>(mouse.x), static_cast<int>(mouse.y) };
	input.wheel = static_cast<int>(GetMouseWheelMove());

	// raylib hands each typed character out once, so the queue is drained here.
	int pressed = GetCharPressed();
	while (pressed != 0)
	{
		if (pressed >= 32 && pressed < 127)
		{
			input.typed += static_cast<char>(pressed);
		}
		pressed = GetCharPressed();
	}

	input.screenWidth = renderer.get_screen_width();
	input.screenHeight = renderer.get_screen_height();
	return input;
}
