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

#include <cstdlib>

#include <gtest/gtest-spi.h>
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

namespace
{
// Set in the parent just before the child starts; the child inherits it.
constexpr const char* RERUN_SKIP_MARKER = "HIDDEN_RENDERER_RERUN_SKIP";

// Whether the marker is set in this process's environment.
//
// Example:
//   is_rerun_skip_marker_set(); // -> false in the parent before the call
//   is_rerun_skip_marker_set(); // -> true in the child the call starts
bool is_rerun_skip_marker_set()
{
	char* value = nullptr;
	size_t length = 0;
	_dupenv_s(&value, &length, RERUN_SKIP_MARKER);
	const bool isSet = value != nullptr && value[0] != '\0';
	std::free(value);
	return isSet;
}

// Starts a child whose re-run of the TEST skips before it reaches the call,
// so the child exits 0 without running its body.
//
// Example:
//   run_a_child_whose_rerun_skips(); // the harness reports one failure
void run_a_child_whose_rerun_skips()
{
	_putenv_s(RERUN_SKIP_MARKER, "1");
	exit_code_in_child(&open_and_close_hidden_renderer);
	// An empty value removes the variable, so later tests never see it.
	_putenv_s(RERUN_SKIP_MARKER, "");
}
} // namespace

// A death test's child re-runs the whole TEST, so a skip before the call
// ends the child with 0 before its body runs. That 0 is never a pass.
TEST(HeadlessRendererTest, AChildWhoseRerunSkipsBeforeTheCallFails)
{
	if (is_rerun_skip_marker_set())
	{
		GTEST_SKIP() << "the child's re-run, which skips by design";
	}
	EXPECT_NONFATAL_FAILURE(
		run_a_child_whose_rerun_skips(),
		"without finishing its body");
}

// end of file: HeadlessRendererTest.cpp
