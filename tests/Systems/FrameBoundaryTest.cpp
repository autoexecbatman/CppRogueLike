// file: FrameBoundaryTest.cpp
// One pass of Game::tick spends the input it reads. Raylib moves a press into the past
// only when a frame ends, so a tick that reads a key and ends no frame leaves the key a
// press for whatever reads next - on the web build, a scroll's Enter that also confirmed
// the targeting cursor opened by that same Enter.
//
// Each test runs in a child process with tests/mocks/HiddenRenderer.h, and skips with the
// reason where there is no OpenGL driver, which is the CI runner today. Input reaches
// raylib through PlayAutomationEvent, the way a real key or click does, because the
// scripted keys of InputSystem never consult raylib's frame state.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=FrameBoundaryTest.*

#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <raylib.h>

#include "src/CharacterSheetUI.h"
#include "src/ContextMenu.h"
#include "src/Game.h"
#include "src/GameContext.h"
#include "src/Player.h"
#include "src/Vector2D.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/TestDice.h"

namespace
{
// Raylib keeps AutomationEventType inside rcore.c; these are its values in raylib 5.5.
constexpr unsigned int AUTOMATION_KEY_DOWN = 2;
constexpr unsigned int AUTOMATION_MOUSE_BUTTON_DOWN = 6;

// Presses a key as the keyboard would: its state goes down and it joins the press queue.
//
// Example:
//   press_key(KEY_ESCAPE);
//   IsKeyPressed(KEY_ESCAPE); // -> true until a frame ends
void press_key(int raylibKey)
{
	PlayAutomationEvent(AutomationEvent{ .frame = 0, .type = AUTOMATION_KEY_DOWN, .params = { raylibKey, 0, 0, 0 } });
}

// Presses a mouse button as the mouse would.
//
// Example:
//   press_mouse_button(MOUSE_BUTTON_LEFT);
//   IsMouseButtonPressed(MOUSE_BUTTON_LEFT); // -> true until a frame ends
void press_mouse_button(int raylibButton)
{
	PlayAutomationEvent(AutomationEvent{ .frame = 0, .type = AUTOMATION_MOUSE_BUTTON_DOWN, .params = { raylibButton, 0, 0, 0 } });
}

// The character sheet closes on Escape without drawing; the tick still ends its frame,
// so Escape is no longer a press when the game reads input next.
[[noreturn]] void escape_closing_the_sheet_is_spent_by_the_tick()
{
	HiddenRenderer hidden{};
	Game game{ TEST_DICE_SEED };
	const Player reader{ Vector2D{ 1, 1 } };
	game.menus.push_back(std::make_unique<CharacterSheetUI>(reader));

	press_key(KEY_ESCAPE);
	exit_unless(IsKeyPressed(KEY_ESCAPE), "the played Escape did not register as a press, so this test measures nothing");

	int loopNum{ 0 };
	game.tick(loopNum);

	exit_unless(game.menus.empty(), "the character sheet did not close on Escape");
	exit_unless(!IsKeyPressed(KEY_ESCAPE), "Escape is still a press after the tick that read it, so the next reader takes it again");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// The callback a context menu runs in these tests: it records that a choice
// was made, in the flag it was given.
//
// Example:
//   bool chosen{ false };
//   ChoiceRecorder{ chosen }(0, ctx); // chosen -> true
struct ChoiceRecorder
{
	bool& chosen;

	void operator()(
		[[maybe_unused]] int choice,
		[[maybe_unused]] GameContext& choiceCtx) const
	{
		chosen = true;
	}
};

// A click chooses from the context menu, which closes without drawing; the tick still
// ends its frame, so the click is not read a second time by the menu beneath.
[[noreturn]] void a_click_closing_the_context_menu_is_spent_by_the_tick()
{
	HiddenRenderer hidden{};
	Game game{ TEST_DICE_SEED };
	GameContext ctx = game.context();
	bool chosen{ false };
	const ChoiceRecorder record_choice{ chosen };
	game.menus.push_back(std::make_unique<ContextMenu>(std::vector<std::string>{ "Use" }, 1, 1, record_choice, ctx));

	press_mouse_button(MOUSE_BUTTON_LEFT);
	exit_unless(IsMouseButtonPressed(MOUSE_BUTTON_LEFT), "the played click did not register as a press, so this test measures nothing");

	int loopNum{ 0 };
	game.tick(loopNum);

	exit_unless(chosen, "the click did not reach the context menu");
	exit_unless(game.menus.empty(), "the context menu did not close on the click");
	exit_unless(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT), "the click is still a press after the tick that read it, so the next reader takes it again");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}
} // namespace

TEST(FrameBoundaryTest, AKeyThatClosesAMenuIsSpentByTheTick)
{
	const int childExitCode = exit_code_in_child(&escape_closing_the_sheet_is_spent_by_the_tick);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED) << "the child's check that failed is in its output above";
}

TEST(FrameBoundaryTest, AClickThatClosesAMenuIsSpentByTheTick)
{
	const int childExitCode = exit_code_in_child(&a_click_closing_the_context_menu_is_spent_by_the_tick);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED) << "the child's check that failed is in its output above";
}

// end of file: FrameBoundaryTest.cpp
