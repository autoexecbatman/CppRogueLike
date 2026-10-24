#pragma once

// file: EditorInput.h
//
// What a data editor's frame reads from the keyboard and mouse, taken once per
// frame as a value. The editor's input handling reads only this snapshot, so a
// test builds one by hand instead of pressing real keys.
//
// Usage:
//
//   // in the editor's tick: the real input, read from raylib once
//   const EditorInput input = read_editor_input(*ctx.renderer);
//
//   // in a test: Ctrl held and Right pressed, the mouse parked in the corner
//   EditorInput input{};
//   input.pressed = { EditorKey::RIGHT };   // keys that went down this frame
//   input.held = { EditorKey::CONTROL };    // keys held down, pressed or not
//   input.screenHeight = 900;               // the window's height in pixels
//   input.is_pressed(EditorKey::RIGHT);     // -> true
//   input.is_held(EditorKey::CONTROL);      // -> true
//   input.is_pressed(EditorKey::LEFT);      // -> false

#include <set>
#include <string>

#include "Vector2D.h"

class Renderer;

// A key or button an editor reacts to.
enum class EditorKey
{
	ESCAPE,
	TAB,
	ENTER,
	UP,
	DOWN,
	LEFT,
	RIGHT,
	// F2, which opens the tile picker.
	PICKER,
	// A, which adds a record.
	ADD,
	// S, which saves when Control is held.
	SAVE,
	// Delete, which removes a custom record.
	REMOVE,
	BACKSPACE,
	// Either Control key; only ever held.
	CONTROL,
	// The left mouse button.
	CLICK
};

// One frame's input, as the editor sees it.
struct EditorInput
{
	// The keys and buttons that went down this frame.
	std::set<EditorKey> pressed{};
	// The keys held down this frame, whether or not they went down in it.
	std::set<EditorKey> held{};
	// Where the mouse is, in screen pixels.
	Vector2D mouse{};
	// How far the wheel turned this frame, in notches; positive is away from
	// the user.
	int wheel{ 0 };
	// The printable characters typed this frame, in order.
	std::string typed{};
	// The window's width in pixels.
	int screenWidth{ 0 };
	// The window's height in pixels.
	int screenHeight{ 0 };

	// Whether the key went down this frame.
	//
	// Example:
	//   const EditorInput tab{ .pressed = { EditorKey::TAB } };
	//   tab.is_pressed(EditorKey::TAB);   // -> true
	[[nodiscard]] bool is_pressed(EditorKey key) const
	{
		return pressed.contains(key);
	}

	// Whether the key is held down this frame.
	//
	// Example:
	//   EditorInput{}.is_held(EditorKey::CONTROL);   // -> false
	[[nodiscard]] bool is_held(EditorKey key) const
	{
		return held.contains(key);
	}
};

// This frame's real input, read from raylib, with the screen size the renderer
// reports. Call it once per frame: raylib hands each typed character out once.
//
// Example:
//   read_editor_input(renderer).screenHeight;
//   // -> renderer.get_screen_height()
[[nodiscard]] EditorInput read_editor_input(const Renderer& renderer);
