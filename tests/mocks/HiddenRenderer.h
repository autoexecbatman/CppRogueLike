#pragma once

// file: HiddenRenderer.h
//
// A renderer for the tests that ask for one: a hidden raylib window, the game font,
// the DawnLike tile sheets and a viewport, built by a test and gone when it ends. The
// plain MockGameContext keeps ctx.renderer null, by the owner's ruling of 2026-10-17;
// there is no process-wide window and nothing here is global.
//
// It only ever runs in a child process. Without an OpenGL driver - the CI runner has
// none - raylib's InitWindow does not fail cleanly: the debug GLFW asserts and aborts.
// The child watches raylib's log for GLFW's "API unavailable" error (65542), which
// arrives just before that abort, and exits with HIDDEN_RENDERER_NO_OPENGL_DRIVER
// instead, so the parent can skip with the reason rather than fail.
//
// Usage. The test's body is a function that runs in the child and ends in an exit
// code; the parent reads the code and skips, passes or fails on it:
//
//   [[noreturn]] void sell_through_the_menu()               // runs in the child
//   {
//       HiddenRenderer hidden{};                            // the window, font and tiles
//       MockGameContext mock{};
//       GameContext ctx = mock.to_game_context();
//       ctx.renderer = &hidden.renderer;                    // only this test has one
//       ...                                                 // build the menu, drive it
//       exit_unless(sold, "the menu did not sell the item"); // stderr, then exit 43
//       std::_Exit(HIDDEN_RENDERER_PASSED);
//   }
//
//   TEST(SellMenuTest, ...)
//   {
//       const int childExitCode = exit_code_in_child(&sell_through_the_menu);
//       if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
//       {
//           GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;         // CI, with no driver
//       }
//       EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED);    // the child's message says why
//   }
//
// A GoogleTest expectation made inside the child is not reported to the parent, so a
// child checks with exit_unless. An assertion probe opens a renderer in a first child
// with open_and_close_hidden_renderer to learn whether it can, and only then expects
// the death; without the first, a missing driver reads as a probe matching the wrong
// message.
//
// The renderer's colour pairs are not filled - only Renderer::init does that, and init
// opens a full-screen window of its own - so text draws in the empty pair. Nothing a
// test checks reads a colour.

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include <gtest/gtest.h>
#include <raylib.h>

#include "src/Paths.h"
#include "src/Renderer.h"

// What a child reports. Distinct from the codes an abort or a crash gives.
inline constexpr int HIDDEN_RENDERER_PASSED = 0;
inline constexpr int HIDDEN_RENDERER_NO_OPENGL_DRIVER = 40;
inline constexpr int HIDDEN_RENDERER_WINDOW_NOT_READY = 41;
inline constexpr int HIDDEN_RENDERER_FONT_FELL_BACK = 42;
inline constexpr int HIDDEN_RENDERER_EXPECTATION_FAILED = 43;

// What a test says when it skips for the missing driver.
inline constexpr std::string_view NO_OPENGL_DRIVER_REASON = "no OpenGL driver on this machine (GLFW error 65542, API unavailable), so a raylib window cannot open";

// The window's size: twenty tiles by fourteen at the game's 64-pixel tiles, enough for
// every menu to lay out. Hidden, so nothing appears on the desktop.
inline constexpr int HIDDEN_WINDOW_WIDTH = 1280;
inline constexpr int HIDDEN_WINDOW_HEIGHT = 900;
// The size main.cpp loads the game font at.
inline constexpr int HIDDEN_RENDERER_FONT_SIZE = 30;

// GLFW's GLFW_API_UNAVAILABLE, as raylib's log prints it when the driver has no OpenGL.
inline constexpr std::string_view NO_OPENGL_ERROR = "GLFW: Error: 65542";

// Raylib's log, echoed to stderr and watched for the one line that means there is no
// OpenGL driver. The child exits on it, before the assert that follows it in InitWindow.
//
// Example:
//   SetTraceLogCallback(exit_on_missing_opengl);
//   InitWindow(...); // on a machine with no driver: exits 40 here
inline void exit_on_missing_opengl(int logLevel, const char* text, va_list args)
{
	char line[512]{};
	std::vsnprintf(line, sizeof(line), text, args);
	std::fprintf(stderr, "raylib [%d]: %s\n", logLevel, line);
	if (std::string_view{ line }.starts_with(NO_OPENGL_ERROR))
	{
		std::fflush(stderr);
		std::_Exit(HIDDEN_RENDERER_NO_OPENGL_DRIVER);
	}
}

// In a child, a check that ends the child when it fails: the message to stderr, where
// the parent's output shows it, then exit 43.
//
// Example:
//   exit_unless(player.get_gold() == 12, "the seller was not paid"); // passes, returns
//   exit_unless(false, "never true"); // prints "never true", exits 43
inline void exit_unless(bool holds, std::string_view what)
{
	if (!holds)
	{
		std::fprintf(stderr, "child check failed: %.*s\n", static_cast<int>(what.size()), what.data());
		std::fflush(stderr);
		std::_Exit(HIDDEN_RENDERER_EXPECTATION_FAILED);
	}
}

// A hidden window and a renderer that draws into it, for one child's lifetime. Builds or
// exits: 40 with no OpenGL driver, 41 if the window does not come up, 42 if the game font
// did not load and measuring falls back to raylib's default.
//
// Example, in a child:
//   HiddenRenderer hidden{};
//   hidden.renderer.get_viewport_cols(); // -> 20
//   hidden.renderer.measure_text("Hello"); // -> the game font's width, not the default's
class HiddenRenderer
{
public:
	Renderer renderer{};

	HiddenRenderer()
	{
		SetTraceLogCallback(exit_on_missing_opengl);
		SetConfigFlags(FLAG_WINDOW_HIDDEN);
		InitWindow(HIDDEN_WINDOW_WIDTH, HIDDEN_WINDOW_HEIGHT, "hidden renderer");
		if (!IsWindowReady())
		{
			std::_Exit(HIDDEN_RENDERER_WINDOW_NOT_READY);
		}

		renderer.load_font(Paths::resolve(Paths::DAWNLIKE_FONT).string(), HIDDEN_RENDERER_FONT_SIZE);
		renderer.load_dawnlike(Paths::resolve(Paths::DAWNLIKE_DIR).string());
		// The viewport is read from the window, as Renderer::init does after opening its own.
		renderer.update_viewport();

		// measure_text falls back to raylib's default font when the game font did not
		// load, so a measure equal to the default's means the game font is not in use.
		if (renderer.measure_text("Hello") == MeasureText("Hello", HIDDEN_RENDERER_FONT_SIZE))
		{
			std::_Exit(HIDDEN_RENDERER_FONT_FELL_BACK);
		}
	}

	// Every load paired with its unload while the context is alive; shutdown leaves the
	// window alone because the renderer did not open it.
	~HiddenRenderer()
	{
		renderer.shutdown();
		CloseWindow();
	}

	HiddenRenderer(const HiddenRenderer&) = delete;
	HiddenRenderer& operator=(const HiddenRenderer&) = delete;
	HiddenRenderer(HiddenRenderer&&) = delete;
	HiddenRenderer& operator=(HiddenRenderer&&) = delete;
};

// A child body that opens a renderer, closes it, and passes: whether one can be built here.
//
// Example:
//   exit_code_in_child(&open_and_close_hidden_renderer); // -> 0 locally, 40 on CI
[[noreturn]] inline void open_and_close_hidden_renderer()
{
	{
		HiddenRenderer hidden{};
	}
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// Hands a child's exit code back to the parent, which decides between a skip and a failure.
class RecordExitCode
{
private:
	int* exitCode{ nullptr };

public:
	explicit RecordExitCode(int* destination)
		: exitCode(destination)
	{
	}

	bool operator()(int childExitCode) const
	{
		*exitCode = childExitCode;
		return true;
	}
};

// Runs a child body in its own process and returns the code it exited with.
//
// Example:
//   exit_code_in_child(&open_and_close_hidden_renderer); // -> 0, or 40 with no driver
inline int exit_code_in_child(void (*childBody)())
{
	int childExitCode = -1;
	EXPECT_EXIT(childBody(), RecordExitCode{ &childExitCode }, "");
	return childExitCode;
}

// end of file: HiddenRenderer.h
