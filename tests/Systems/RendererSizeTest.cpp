// file: RendererSizeTest.cpp
// The renderer reports the size the window has, whatever size was asked for.
// Windows can give a window less than requested, a fullscreen request loses the
// taskbar's rows, and no resize event says so; every layout that reads the
// renderer's size then lays out past the window's bottom edge.
//
// It runs in a child with a hidden window (tests/mocks/HiddenRenderer.h), and
// skips where no OpenGL driver exists, as on the CI runner.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RendererSizeTest.*

#include <gtest/gtest.h>

#include <raylib.h>

#include "tests/mocks/HiddenRenderer.h"

namespace
{
	// A size no tile size divides, so a stale or rounded value cannot match it.
	constexpr int RESIZED_WIDTH = HIDDEN_WINDOW_WIDTH - 45;
	constexpr int RESIZED_HEIGHT = HIDDEN_WINDOW_HEIGHT - 19;

	// Resizes the window with no resize event, then compares the renderer with
	// it.
	//
	// Example:
	//   exit_code_in_child(&the_renderer_reports_the_resized_window);
	//   // -> 44 when every size matches, 43 naming the first that does not
	[[noreturn]] void the_renderer_reports_the_resized_window()
	{
		HiddenRenderer hidden{};
		SetWindowSize(RESIZED_WIDTH, RESIZED_HEIGHT);
		exit_unless(
			GetScreenWidth() == RESIZED_WIDTH &&
				GetScreenHeight() == RESIZED_HEIGHT,
			"the window did not take the new size, so nothing was measured");

		exit_unless(
			hidden.renderer.get_screen_width() == RESIZED_WIDTH,
			"get_screen_width is not the window's width");
		exit_unless(
			hidden.renderer.get_screen_height() == RESIZED_HEIGHT,
			"get_screen_height is not the window's height");
		const int tileSize = hidden.renderer.get_tile_size();
		exit_unless(
			hidden.renderer.get_viewport_cols() == RESIZED_WIDTH / tileSize,
			"get_viewport_cols is not the whole tiles across the window");
		exit_unless(
			hidden.renderer.get_viewport_rows() == RESIZED_HEIGHT / tileSize,
			"get_viewport_rows is not the whole tiles down the window");

		// A zoom changes the tile size, and the tile counts follow it.
		hidden.renderer.zoom_out();
		const int zoomedTileSize = hidden.renderer.get_tile_size();
		exit_unless(
			zoomedTileSize != tileSize,
			"zoom_out kept the tile size, so the zoom was not measured");
		exit_unless(
			hidden.renderer.get_viewport_cols() ==
				RESIZED_WIDTH / zoomedTileSize,
			"get_viewport_cols did not follow the zoom");
		exit_unless(
			hidden.renderer.get_viewport_rows() ==
				RESIZED_HEIGHT / zoomedTileSize,
			"get_viewport_rows did not follow the zoom");
		finish_the_child();
	}
} // namespace

TEST(RendererSizeTest, TheRendererReportsTheWindowItHasNotTheOneAskedFor)
{
	const int childExitCode =
		exit_code_in_child(&the_renderer_reports_the_resized_window);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}
