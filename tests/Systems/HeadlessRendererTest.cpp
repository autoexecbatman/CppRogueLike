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
// The later tests check that harness: a child that never ran its body, or
// that failed a check, fails the parent's test and carries its text there.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HeadlessRendererTest.*

#include <cassert>
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
// Markers the parent sets just before a child starts; the child inherits them.
constexpr const char* RERUN_SKIP_MARKER = "HIDDEN_RENDERER_RERUN_SKIP";
constexpr const char* BARE_CHILD_MARKER = "HIDDEN_RENDERER_BARE_CHILD";

// Whether a marker is set in this process's environment.
//
// Example:
//   is_marker_set(RERUN_SKIP_MARKER); // -> false in the parent before the call
//   is_marker_set(RERUN_SKIP_MARKER); // -> true in the child the call starts
bool is_marker_set(const char* marker)
{
	char* value = nullptr;
	size_t length = 0;
	_dupenv_s(&value, &length, marker);
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
	if (is_marker_set(RERUN_SKIP_MARKER))
	{
		GTEST_SKIP() << "the child's re-run, which skips by design";
	}
	EXPECT_NONFATAL_FAILURE(
		run_a_child_whose_rerun_skips(),
		"without finishing its body");
}

namespace
{
// A child whose own expectation fails, then finishes as a passing body would.
//
// Example:
//   exit_code_in_child(&fail_an_expectation_then_finish);
//   // -> 43, with the expectation's text
[[noreturn]] void fail_an_expectation_then_finish()
{
	ADD_FAILURE() << "an expectation made inside the child";
	finish_the_child();
}

// Starts that child with the bare-child marker set, so the child's re-run of
// the TEST calls it outside EXPECT_NONFATAL_FAILURE, whose reporter would
// otherwise take the child's failure before finish_the_child could read it.
//
// Example:
//   run_a_bare_child_failing_an_expectation(); // one failure, with the text
void run_a_bare_child_failing_an_expectation()
{
	_putenv_s(BARE_CHILD_MARKER, "1");
	exit_code_in_child(&fail_an_expectation_then_finish);
	_putenv_s(BARE_CHILD_MARKER, "");
}

// A child whose exit_unless check fails.
//
// Example:
//   exit_code_in_child(&fail_an_exit_unless); // -> 43, with "never true"
[[noreturn]] void fail_an_exit_unless()
{
	exit_unless(false, "never true");
	finish_the_child();
}

// A child that aborts on an assertion; in Release the assert is compiled out.
//
// Example:
//   exit_code_in_child(&abort_on_an_assert); // Debug: -> 3, with "probe text"
[[noreturn]] void abort_on_an_assert()
{
	assert(false && "probe text");
	finish_the_child();
}

// A child that reports the missing driver, as HiddenRenderer does on CI.
//
// Example:
//   exit_code_in_child(&report_no_opengl_driver); // -> 40, and no failure
[[noreturn]] void report_no_opengl_driver()
{
	std::_Exit(HIDDEN_RENDERER_NO_OPENGL_DRIVER);
}
} // namespace

// A gtest expectation inside a child is never printed there, so the harness
// carries its text out in the test's own failure.
TEST(HeadlessRendererTest, AFailedExpectationInTheChildFailsWithItsText)
{
	if (is_marker_set(BARE_CHILD_MARKER))
	{
		// The child's re-run: the same call, with gtest's own reporter.
		exit_code_in_child(&fail_an_expectation_then_finish);
		return;
	}
	EXPECT_NONFATAL_FAILURE(
		run_a_bare_child_failing_an_expectation(),
		"an expectation made inside the child");
}

TEST(HeadlessRendererTest, AFailedExitUnlessFailsWithItsText)
{
	EXPECT_NONFATAL_FAILURE(
		exit_code_in_child(&fail_an_exit_unless),
		"never true");
}

TEST(HeadlessRendererTest, AnAssertInTheChildFailsWithItsText)
{
#ifdef NDEBUG
	GTEST_SKIP() << "Release compiles the assert out, so the child passes";
#endif
	EXPECT_NONFATAL_FAILURE(
		exit_code_in_child(&abort_on_an_assert),
		"probe text");
}

// The skip code is the parent's to read; it is never a failure of its own.
TEST(HeadlessRendererTest, ANoDriverChildIsNotAFailure)
{
	EXPECT_EQ(
		exit_code_in_child(&report_no_opengl_driver),
		HIDDEN_RENDERER_NO_OPENGL_DRIVER);
}

// end of file: HeadlessRendererTest.cpp
