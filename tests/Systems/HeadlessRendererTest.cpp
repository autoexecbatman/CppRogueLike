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
// window and font checks are enforced and a failure fails the test.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HeadlessRendererTest.*

#include <gtest/gtest.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string_view>

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

// What the child reports. Distinct from 0 and from the codes an abort or a crash gives.
constexpr int PROBE_PASSED = 0;
constexpr int PROBE_NO_OPENGL_DRIVER = 40;
constexpr int PROBE_WINDOW_NOT_READY = 41;
constexpr int PROBE_FONT_FELL_BACK = 42;

// GLFW's GLFW_API_UNAVAILABLE, as raylib's log prints it when the driver has no OpenGL.
constexpr std::string_view NO_OPENGL_ERROR = "GLFW: Error: 65542";

// Raylib's log, watched for the one line that means there is no OpenGL driver. The child
// exits on it, before the assert that follows it in InitWindow can abort.
void exit_on_missing_opengl(int logLevel, const char* text, va_list args)
{
	char line[512]{};
	std::vsnprintf(line, sizeof(line), text, args);
	std::fprintf(stderr, "raylib [%d]: %s\n", logLevel, line);
	if (std::string_view{ line }.starts_with(NO_OPENGL_ERROR))
	{
		std::fflush(stderr);
		std::_Exit(PROBE_NO_OPENGL_DRIVER);
	}
}

// Everything the probe does, ending in an exit code. Runs only in the child process.
[[noreturn]] void run_headless_probe()
{
	SetTraceLogCallback(exit_on_missing_opengl);
	SetConfigFlags(FLAG_WINDOW_HIDDEN);
	InitWindow(PROBE_WINDOW_WIDTH, PROBE_WINDOW_HEIGHT, "headless renderer test");
	if (!IsWindowReady())
	{
		std::_Exit(PROBE_WINDOW_NOT_READY);
	}

	Renderer renderer{};
	renderer.load_font(Paths::resolve(Paths::DAWNLIKE_FONT).string(), GAME_FONT_SIZE);
	renderer.load_dawnlike(Paths::resolve(Paths::DAWNLIKE_DIR).string());

	// measure_text falls back to raylib's default font when the game font did not load, so
	// a measure equal to the default's at the same size means the game font is not in use.
	const bool isGameFontInUse = renderer.measure_text("Hello") != MeasureText("Hello", GAME_FONT_SIZE);

	// Every load paired with its unload while the context is alive; shutdown leaves the
	// window alone because the renderer did not open it.
	renderer.shutdown();
	CloseWindow();
	std::_Exit(isGameFontInUse ? PROBE_PASSED : PROBE_FONT_FELL_BACK);
}

// Hands the child's exit code back to the test, which decides between a skip and a failure.
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
} // namespace

TEST(HeadlessRendererTest, AHiddenWindowComesUpAndTheFontMeasuresText)
{
	int childExitCode = -1;
	EXPECT_EXIT(run_headless_probe(), RecordExitCode{ &childExitCode }, "");

	if (childExitCode == PROBE_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << "no OpenGL driver on this machine (GLFW error 65542, API unavailable), so a raylib window cannot open";
	}
	ASSERT_NE(childExitCode, PROBE_WINDOW_NOT_READY) << "a hidden raylib window could not be opened, with an OpenGL driver present";
	ASSERT_NE(childExitCode, PROBE_FONT_FELL_BACK) << "the game font did not load in a hidden window; the measure is raylib's default font";
	EXPECT_EQ(childExitCode, PROBE_PASSED) << "the probe ended abnormally; its log is in the child's output above";
}

// end of file: HeadlessRendererTest.cpp
