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

#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/Game.h"
#include "src/GameContext.h"
#include "src/GameStateManager.h"
#include "src/InventoryData.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/PlayerRaceState.h"
#include "src/TurnSchedule.h"
#include "src/UniqueId.h"
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
			GTEST_SKIP() << std::format(
				"a save already exists at {}; refusing to overwrite it to run this test",
				Paths::resolve(Paths::SAVE_FILE).string());
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
		ctx.gameState->advance_clock(
			ROUNDS_BEFORE_SAVING * TIME_UNITS_PER_ROUND);
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
		ASSERT_TRUE(saveDocument.contains(std::string{ key }))
			<< "the writer did not write \"" << key
			<< "\", so erasing it proves nothing";
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
		ASSERT_TRUE(saveDocument.at("player").contains("position"))
			<< "the player was saved without a position, so erasing it proves nothing";
		saveDocument.at("player").erase("position");
		std::ofstream out{ savePath };
		out << saveDocument.dump(4);
	}

	// Rewrites the save's id counter to the id its player holds, as though it had fallen behind.
	static void set_the_saved_counter_to_the_players_id()
	{
		const std::filesystem::path savePath = Paths::resolve(Paths::SAVE_FILE);
		nlohmann::json saveDocument;
		{
			std::ifstream in{ savePath };
			in >> saveDocument;
		}
		saveDocument.at("nextUniqueId") =
			saveDocument.at("player").at("uniqueId");
		std::ofstream out{ savePath };
		out << saveDocument.dump(4);
	}

	// Every id an object of the loaded game holds: the player, the creatures, and what each carries or lies on the floor.
	std::unordered_set<UniqueId::IdType> held_ids()
	{
		std::unordered_set<UniqueId::IdType> ids{ ctx.player()->uniqueId };
		for (const auto& item : ctx.player()->inventoryData.items)
		{
			assert(item && "held_ids: the player's pack holds a null entry");
			ids.insert(item->uniqueId);
		}
		for (const auto& creature : *ctx.creatures)
		{
			assert(
				creature && "held_ids: the creature list holds a null entry");
			ids.insert(creature->uniqueId);
			for (const auto& item : creature->inventoryData.items)
			{
				assert(
					item && "held_ids: a creature's pack holds a null entry");
				ids.insert(item->uniqueId);
			}
		}
		for (const auto& item : ctx.floorInventory->items)
		{
			assert(item && "held_ids: the floor holds a null entry");
			ids.insert(item->uniqueId);
		}
		return ids;
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
class SaveRequiredKeysTest :
	public SaveRequiredKeysFixture,
	public ::testing::WithParamInterface<std::string_view>
{
};

// The control: the fixture's save loads whole, so a refusal below comes from the erased key.
TEST_F(SaveRequiredKeysFixture, AnUntouchedSaveLoadsWithItsClock)
{
	save_a_game_in_progress();
	ASSERT_TRUE(wroteSave)
		<< "the game could not be saved, so nothing here is being measured";

	const LoadOutcome outcome = game.stateManager.load_all(ctx);

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::LOADED) << outcome.reason;
	EXPECT_EQ(ctx.gameState->get_time(), savedTime)
		<< "the loaded game did not keep its saved clock";
}

// One level down: Actor::load's keys are required too, and the whole load refuses.
TEST_F(SaveRequiredKeysFixture, ASaveWhosePlayerHasNoPositionIsRefused)
{
	save_a_game_in_progress();
	ASSERT_TRUE(wroteSave)
		<< "the game could not be saved, so nothing here is being measured";
	erase_the_players_position();

	const LoadOutcome outcome = game.stateManager.load_all(ctx);

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::REFUSED)
		<< "a player without a position was not refused";
	EXPECT_NE(outcome.reason.find("position"), std::string::npos)
		<< "the reason does not name the missing key: " << outcome.reason;
	EXPECT_FALSE(save_exists())
		<< "the refused save was kept; the owner's ruling is to delete it";
}

// A saved counter behind an id the save holds is not trusted: the load issued ids of its own
// while building the actors, and the counter resumes past those.
TEST_F(
	SaveRequiredKeysFixture,
	AnObjectMadeAfterLoadingASaveWithAStaleCounterTakesAnUnheldId)
{
	save_a_game_in_progress();
	ASSERT_TRUE(wroteSave)
		<< "the game could not be saved, so nothing here is being measured";
	set_the_saved_counter_to_the_players_id();

	const LoadOutcome outcome = game.stateManager.load_all(ctx);
	ASSERT_EQ(outcome.kind, LoadOutcomeKind::LOADED) << outcome.reason;
	const std::unordered_set<UniqueId::IdType> held = held_ids();
	const auto made = ItemCreator::create("dagger", Vector2D{ 0, 0 }, ctx);

	EXPECT_FALSE(held.contains(made->uniqueId))
		<< "an object made after the load took id " << made->uniqueId
		<< ", which a loaded object already holds";
}

TEST_P(SaveRequiredKeysTest, ASaveMissingTheKeyIsRefusedNamingItAndDeleted)
{
	const std::string_view key = GetParam();
	save_a_game_in_progress();
	ASSERT_TRUE(wroteSave)
		<< "the game could not be saved, so nothing here is being measured";
	erase_from_the_save(key);

	const LoadOutcome outcome = game.stateManager.load_all(ctx);

	EXPECT_EQ(outcome.kind, LoadOutcomeKind::REFUSED)
		<< "a save without \"" << key << "\" was not refused";
	EXPECT_NE(outcome.reason.find(key), std::string::npos)
		<< "the reason does not name the missing key: " << outcome.reason;
	EXPECT_FALSE(save_exists())
		<< "the refused save was kept; the owner's ruling is to delete it";
}

// Each case is named for its key, so a failure says which one without reading the output.
const auto name_for_key =
	[](const ::testing::TestParamInfo<std::string_view>& info)
{
	return std::string{ info.param };
};

INSTANTIATE_TEST_SUITE_P(
	EveryKeyTheWriterAlwaysWrites,
	SaveRequiredKeysTest,
	::testing::Values(
		"player",
		"stairs",
		"gui",
		"hunger_system",
		"time",
		"roundsRun",
		"nextUniqueId"),
	name_for_key);
