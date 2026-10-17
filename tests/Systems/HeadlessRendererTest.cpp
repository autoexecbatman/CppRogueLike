// file: HeadlessRendererTest.cpp
// Whether the renderer can come up inside the test process: a hidden raylib window, the
// game's font and the DawnLike tile sheets, and the font measuring text. Menus size their
// boxes with measure_text, so no menu can be built in a test until this holds.
//
// It is the measurement for issues/0141, taken where it matters: run on the CI runner, a
// window that cannot open fails here with raylib's own log above it, rather than leaving
// the renderer out of the test context by assumption.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HeadlessRendererTest.*

#include <gtest/gtest.h>

#include <raylib.h>

#include "src/Paths.h"
#include "src/Renderer.h"

namespace
{
// Small and hidden: nothing is drawn, and nothing should appear on the desktop.
constexpr int PROBE_WINDOW_WIDTH = 320;
constexpr int PROBE_WINDOW_HEIGHT = 240;
// The size main.cpp loads the game font at.
constexpr int GAME_FONT_SIZE = 30;
} // namespace

TEST(HeadlessRendererTest, AHiddenWindowComesUpAndTheFontMeasuresText)
{
	SetConfigFlags(FLAG_WINDOW_HIDDEN);
	InitWindow(PROBE_WINDOW_WIDTH, PROBE_WINDOW_HEIGHT, "headless renderer test");
	const bool isWindowReady = IsWindowReady();

	if (isWindowReady)
	{
		Renderer renderer{};
		renderer.load_font(Paths::resolve(Paths::DAWNLIKE_FONT).string(), GAME_FONT_SIZE);
		renderer.load_dawnlike(Paths::resolve(Paths::DAWNLIKE_DIR).string());

		// measure_text falls back to raylib's default font when the game font did not load, so
		// a measure that differs from the default's at the same size is the game font at work.
		EXPECT_NE(renderer.measure_text("Hello"), MeasureText("Hello", GAME_FONT_SIZE))
			<< "the game font did not load in a hidden window; the measure is raylib's default font";

		// Every load paired with its unload while the context is alive; shutdown leaves the
		// window alone because the renderer did not open it.
		renderer.shutdown();
		CloseWindow();
	}

	ASSERT_TRUE(isWindowReady) << "a hidden raylib window could not be opened in the test process";
}

// end of file: HeadlessRendererTest.cpp
