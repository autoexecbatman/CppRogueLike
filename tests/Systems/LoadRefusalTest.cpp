// file: LoadRefusalTest.cpp
// What Load Game does with a save the loaders refuse: report it, delete it, leave no game.
//
// The owner's ruling on issues/0293, "Delete it.": a save the strict loaders refuse is
// reported to the player and removed, and play returns to the main menu rather than to a
// half-loaded game. These tests hold GameStateManager::load_all to that. Each writes its
// own stand-in save, and skips rather than overwrite one that is already there, because
// the suite runs from build/bin/Debug, where a real save may sit.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=LoadRefusalTest.*

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <typeinfo>

#include "src/Controls.h"
#include "src/Game.h"
#include "src/GameContext.h"
#include "src/GameStateManager.h"
#include "src/InputSystem.h"
#include "src/ListMenu.h"
#include "src/Menu.h"
#include "src/MenuManager.h"
#include "src/NotificationMenu.h"
#include "src/Paths.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/TestDice.h"

namespace
{
// Writes a save the loaders refuse where the game keeps its save.
void place_refused_save()
{
	const auto savePath = Paths::resolve(Paths::SAVE_FILE);
	std::error_code ignored;
	std::filesystem::create_directories(savePath.parent_path(), ignored);
	std::ofstream out{ savePath };
	out << "{}";
}

// One frame of the menu stack, opened and closed as Game::tick does around it.
void run_one_menu_frame(Game& game, GameContext& ctx, HiddenRenderer& hidden)
{
	hidden.renderer.begin_frame();
	game.menuManager.handle_menus(game.menus, ctx);
	hidden.renderer.end_frame();
}

// Load Game on a refused save shows the refusal, and a key returns to a main menu alone.
[[noreturn]] void load_game_on_a_refused_save_reports_it_and_returns_to_the_menu()
{
	HiddenRenderer hidden{};
	Game game{ TEST_DICE_SEED };
	GameContext ctx = game.context();
	ctx.renderer = &hidden.renderer;
	game.tileConfig.load(Paths::TILE_CONFIG);

	// Written here and not in the TEST body, which the child re-runs: a save there makes the child skip.
	place_refused_save();
	game.menus.push_back(make_main_menu(true, ctx));
	// Load Game is the second entry, offered because the save exists.
	ctx.inputSystem->queue_key(GameKey::DOWN, 0);
	run_one_menu_frame(game, ctx, hidden);
	ctx.inputSystem->queue_key(GameKey::ENTER, 0);
	run_one_menu_frame(game, ctx, hidden);

	exit_unless(!std::filesystem::exists(Paths::resolve(Paths::SAVE_FILE)), "the refused save was not deleted");
	exit_unless(!game.menuManager.is_game_initialized(), "the refused load left the game marked running");
	exit_unless(!game.menus.empty() && typeid(*game.menus.back()) == typeid(NotificationMenu), "no notice of the refusal is on top of the stack");
	ctx.inputSystem->queue_key(GameKey::ENTER, 0);
	run_one_menu_frame(game, ctx, hidden);

	exit_unless(game.menus.size() == 1, "closing the notice left more than the main menu on the stack");
	exit_unless(typeid(*game.menus.back()) == typeid(ListMenu) && game.menus.back()->run, "closing the notice did not leave a running main menu");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}
} // namespace

class LoadRefusalTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// A real save is never overwritten to prove anything about saves.
		if (std::filesystem::exists(Paths::resolve(Paths::SAVE_FILE)))
		{
			GTEST_SKIP() << "a save already exists at " << Paths::resolve(Paths::SAVE_FILE).string()
						 << "; refusing to overwrite it to run this test";
		}
		game.tileConfig.load(Paths::TILE_CONFIG);
	}

	void TearDown() override
	{
		// Only ever removes a file this fixture wrote, and only if a test left it behind.
		if (wroteStandIn)
		{
			std::error_code ignored;
			std::filesystem::remove(Paths::resolve(Paths::SAVE_FILE), ignored);
		}
	}

	// Writes the stand-in save; wroteStandIn says the write landed.
	void place_save(const std::string& contents)
	{
		const auto savePath = Paths::resolve(Paths::SAVE_FILE);
		std::error_code ignored;
		std::filesystem::create_directories(savePath.parent_path(), ignored);
		std::ofstream out{ savePath };
		out << contents;
		out.close();
		wroteStandIn = std::filesystem::exists(savePath);
	}

	static bool save_exists()
	{
		return std::filesystem::exists(Paths::resolve(Paths::SAVE_FILE));
	}

	Game game{ TEST_DICE_SEED };
	GameContext ctx{ game.context() };
	bool wroteStandIn{ false };
};

TEST_F(LoadRefusalTest, AnUnparseableSaveIsRefusedDeletedAndLeavesTheGameUninitialized)
{
	place_save("LoadRefusalTest stand-in - not json");
	ASSERT_TRUE(wroteStandIn) << "could not write the stand-in save, so nothing here is being measured";

	const LoadOutcome outcome = game.stateManager.load_all(ctx);

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::REFUSED);
	EXPECT_FALSE(outcome.reason.empty()) << "a refusal must carry the loader's reason for the player";
	EXPECT_FALSE(save_exists()) << "the refused save was kept; the owner's ruling is to delete it";
	EXPECT_FALSE(game.menuManager.is_game_initialized()) << "a refused load left the game marked running";
}

TEST_F(LoadRefusalTest, ASaveMissingItsMapIsRefusedAndItsReasonNamesTheKey)
{
	place_save("{}");
	ASSERT_TRUE(wroteStandIn) << "could not write the stand-in save, so nothing here is being measured";

	const LoadOutcome outcome = game.stateManager.load_all(ctx);

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::REFUSED);
	EXPECT_NE(outcome.reason.find("not found"), std::string::npos) << "the reason was: " << outcome.reason;
	EXPECT_FALSE(save_exists()) << "the refused save was kept; the owner's ruling is to delete it";
	EXPECT_FALSE(game.menuManager.is_game_initialized()) << "a refused load left the game marked running";
}

// Windows refuses to delete a file another handle holds open, which is a delete failing at run
// time as a locked or read-only save would; the refusal is still reported, and says the file stays.
TEST_F(LoadRefusalTest, ARefusedSaveThatCannotBeDeletedIsKeptAndSaysSo)
{
	place_save("{}");
	ASSERT_TRUE(wroteStandIn) << "could not write the stand-in save, so nothing here is being measured";

	LoadOutcome outcome{};
	{
		std::ofstream holder{ Paths::resolve(Paths::SAVE_FILE), std::ios::app };
		ASSERT_TRUE(holder.is_open()) << "could not hold the save open, so the delete cannot be made to fail";
		outcome = game.stateManager.load_all(ctx);
	}

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::REFUSED_AND_KEPT);
	EXPECT_FALSE(outcome.reason.empty()) << "a refusal must carry the loader's reason for the player";
	EXPECT_TRUE(save_exists()) << "the delete was expected to fail while the file is held open";
	EXPECT_FALSE(game.menuManager.is_game_initialized()) << "a refused load left the game marked running";
}

// Runs in a child with a hidden window, so it skips where no OpenGL driver exists (issues/0211).
TEST(LoadRefusalMenuTest, LoadGameOnARefusedSaveReportsItAndReturnsToTheMenu)
{
	if (std::filesystem::exists(Paths::resolve(Paths::SAVE_FILE)))
	{
		GTEST_SKIP() << "a save already exists at " << Paths::resolve(Paths::SAVE_FILE).string()
					 << "; refusing to overwrite it to run this test";
	}

	const int childExitCode = exit_code_in_child(&load_game_on_a_refused_save_reports_it_and_returns_to_the_menu);

	// The stand-in is this test's own; a child that stopped early may have left it.
	std::error_code ignored;
	std::filesystem::remove(Paths::resolve(Paths::SAVE_FILE), ignored);
	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED) << "the child's check that failed is in its output above";
}

TEST_F(LoadRefusalTest, AMissingSaveIsReportedAndLeavesTheGameUninitialized)
{
	const LoadOutcome outcome = game.stateManager.load_all(ctx);

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::NO_SAVE);
	EXPECT_TRUE(outcome.reason.empty());
	EXPECT_FALSE(game.menuManager.is_game_initialized()) << "a load with nothing to read left the game marked running";
}
