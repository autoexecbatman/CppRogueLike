// file: SaveRequiredKeysTest.cpp
// A save missing any top-level key the writer always writes is refused, not loaded with
// that part at its default.
//
// What it is for. GameStateManager::save_game writes "player", "stairs", "gui",
// "hunger_system", "time" and "roundsRun" every time, so a save without one is damaged
// rather than old. Loading it at the default used to put the clock at 0, which readies
// every helm of teleportation, or put a blank character in place of the player. A refused
// load is deleted and reported (the owner's ruling on issues/0293).
//
// Each test writes a real save through save_game, and skips rather than overwrite one that
// is already there, because the suite runs from build/bin/Debug, where a real save may sit.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=SaveRequiredKeys*:EveryKeyTheWriterAlwaysWrites*

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "src/CreatureClass.h"
#include "src/Game.h"
#include "src/GameContext.h"
#include "src/GameStateManager.h"
#include "src/Paths.h"
#include "src/PlayerRaceState.h"
#include "src/TurnSchedule.h"
#include "tests/mocks/TestDice.h"

namespace
{
// Enough rounds that a clock loaded at its default cannot pass for the saved one.
constexpr int ROUNDS_BEFORE_SAVING = 5;
} // namespace

// A game with its registries loaded, ready to start a run and save it.
class SaveRequiredKeysFixture : public ::testing::Test
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
		game.bodyPlanRegistry.load(Paths::BODY_PLANS);
		game.spellRegistry.load(Paths::SPELLS);
		game.monsterRegistry.load(Paths::MONSTERS);
		game.itemRegistry.load(Paths::ITEMS);
		game.itemRegistry.load_enhanced_rules(Paths::ENHANCED_RULES);
		game.init_world();
		game.palette.load(Paths::TILE_CONFIG);
		game.prefabLibrary.load(Paths::PREFABS);
	}

	void TearDown() override
	{
		// Only ever removes a file this fixture wrote, and only if a test left it behind.
		if (wroteSave)
		{
			std::error_code ignored;
			std::filesystem::remove(Paths::resolve(Paths::SAVE_FILE), ignored);
		}
	}

	// New Game as a fighter, a few rounds on, saved through the game's own writer.
	void save_a_game_in_progress()
	{
		game.playerBlueprint.creatureClass = CreatureClass::FIGHTER;
		game.playerBlueprint.playerRaceState = PlayerRaceState::HUMAN;
		game.stateManager.init_new_game(ctx);
		ctx.gameState->advance_clock(ROUNDS_BEFORE_SAVING * TIME_UNITS_PER_ROUND);
		savedTime = ctx.gameState->get_time();
		game.stateManager.save_game(ctx);
		wroteSave = save_exists();
	}

	// Rewrites the save on disk without one top-level key.
	static void erase_from_the_save(std::string_view key)
	{
		const std::filesystem::path savePath = Paths::resolve(Paths::SAVE_FILE);
		nlohmann::json saveDocument;
		{
			std::ifstream in{ savePath };
			in >> saveDocument;
		}
		ASSERT_TRUE(saveDocument.contains(std::string{ key })) << "the writer did not write \"" << key << "\", so erasing it proves nothing";
		saveDocument.erase(std::string{ key });
		std::ofstream out{ savePath };
		out << saveDocument.dump(4);
	}

	// Rewrites the save on disk with the player's record missing its position.
	static void erase_the_players_position()
	{
		const std::filesystem::path savePath = Paths::resolve(Paths::SAVE_FILE);
		nlohmann::json saveDocument;
		{
			std::ifstream in{ savePath };
			in >> saveDocument;
		}
		ASSERT_TRUE(saveDocument.at("player").contains("position")) << "the player was saved without a position, so erasing it proves nothing";
		saveDocument.at("player").erase("position");
		std::ofstream out{ savePath };
		out << saveDocument.dump(4);
	}

	static bool save_exists()
	{
		return std::filesystem::exists(Paths::resolve(Paths::SAVE_FILE));
	}

	Game game{ TEST_DICE_SEED };
	GameContext ctx{ game.context() };
	int savedTime{ 0 };
	bool wroteSave{ false };
};

// The same game, once per key the writer always writes.
class SaveRequiredKeysTest : public SaveRequiredKeysFixture, public ::testing::WithParamInterface<std::string_view>
{
};

// The control: the fixture's save loads whole, so a refusal below comes from the erased key.
TEST_F(SaveRequiredKeysFixture, AnUntouchedSaveLoadsWithItsClock)
{
	save_a_game_in_progress();
	ASSERT_TRUE(wroteSave) << "the game could not be saved, so nothing here is being measured";

	const LoadOutcome outcome = game.stateManager.load_all(ctx);

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::LOADED) << outcome.reason;
	EXPECT_EQ(ctx.gameState->get_time(), savedTime) << "the loaded game did not keep its saved clock";
}

// One level down: Actor::load's keys are required too, and the whole load refuses.
TEST_F(SaveRequiredKeysFixture, ASaveWhosePlayerHasNoPositionIsRefused)
{
	save_a_game_in_progress();
	ASSERT_TRUE(wroteSave) << "the game could not be saved, so nothing here is being measured";
	erase_the_players_position();

	const LoadOutcome outcome = game.stateManager.load_all(ctx);

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::REFUSED) << "a player without a position was not refused";
	EXPECT_NE(outcome.reason.find("position"), std::string::npos) << "the reason does not name the missing key: " << outcome.reason;
	EXPECT_FALSE(save_exists()) << "the refused save was kept; the owner's ruling is to delete it";
}

TEST_P(SaveRequiredKeysTest, ASaveMissingTheKeyIsRefusedNamingItAndDeleted)
{
	const std::string_view key = GetParam();
	save_a_game_in_progress();
	ASSERT_TRUE(wroteSave) << "the game could not be saved, so nothing here is being measured";
	erase_from_the_save(key);

	const LoadOutcome outcome = game.stateManager.load_all(ctx);

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::REFUSED) << "a save without \"" << key << "\" was not refused";
	EXPECT_NE(outcome.reason.find(key), std::string::npos) << "the reason does not name the missing key: " << outcome.reason;
	EXPECT_FALSE(save_exists()) << "the refused save was kept; the owner's ruling is to delete it";
}

// Each case is named for its key, so a failure says which one without reading the output.
const auto name_for_key = [](const ::testing::TestParamInfo<std::string_view>& info)
{
	return std::string{ info.param };
};

INSTANTIATE_TEST_SUITE_P(
	EveryKeyTheWriterAlwaysWrites,
	SaveRequiredKeysTest,
	::testing::Values("player", "stairs", "gui", "hunger_system", "time", "roundsRun"),
	name_for_key);
