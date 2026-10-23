// file: HeadlessRendererTest.cpp
// Whether the renderer can come up inside the test process: a hidden raylib window, the
// game's font and the DawnLike tile sheets, and the font measuring text. Menus size their
// boxes with measure_text, so no menu can be built in a test until this holds.
//
// It is the measurement for issues/0141, and it runs in a child process. Without an
// OpenGL driver - the CI runner has none - raylib's InitWindow does not fail cleanly: it
// asks GLFW for the position of a window that was never created, and the debug GLFW
// asserts and aborts. The child reports that case through raylib's log, where GLFW's
// "API unavailable" error (65542) arrives just before the abort, and exits with its own
// code; the parent turns it into a skip that names the reason. Where a driver exists, the
// window and font checks are enforced and a failure fails the test. The window and the
// child are tests/mocks/HiddenRenderer.h, which every test needing a renderer shares.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HeadlessRendererTest.*

#include <gtest/gtest.h>

#include "tests/mocks/HiddenRenderer.h"

TEST(HeadlessRendererTest, AHiddenWindowComesUpAndTheFontMeasuresText)
{
	const int childExitCode =
		exit_code_in_child(&open_and_close_hidden_renderer);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	ASSERT_NE(childExitCode, HIDDEN_RENDERER_WINDOW_NOT_READY)
		<< "a hidden raylib window could not be opened, with an OpenGL driver present";
	ASSERT_NE(childExitCode, HIDDEN_RENDERER_FONT_FELL_BACK)
		<< "the game font did not load in a hidden window; the measure is raylib's default font";
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the probe ended abnormally; its log is in the child's output above";
}

// end of file: HeadlessRendererTest.cpp
