// file: NewRunStateTest.cpp
// A run starts from a fresh run state, whichever way it is entered: New Game, or Load Game.
// The last run's hunger, its count of rounds already run, its current message and its
// message log used to carry into the next one (issues/0298). The round count was the worst:
// the clock went back to 0 while the count did not, so the new game ran no round upkeep -
// no hunger, regeneration or poison - until it passed the last run's round count.
//
// Each test plays a first run forward, leaving a marker line in the message log and a part
// of a message still being written, then enters a new run and looks for any of it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=NewRunStateTest.*

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "src/Colors.h"
#include "src/CreatureClass.h"
#include "src/Game.h"
#include "src/GameContext.h"
#include "src/GameLoopCoordinator.h"
#include "src/GameStateManager.h"
#include "src/HungerSystem.h"
#include "src/LogMessage.h"
#include "src/MessageSystem.h"
#include "src/Paths.h"
#include "src/PlayerRaceState.h"
#include "src/TurnSchedule.h"
#include "tests/mocks/TestDice.h"

namespace
{
constexpr std::string_view FIRST_RUN_LINE{ "first run marker line" };
constexpr std::string_view FIRST_RUN_PART{ "first run unfinished part" };
// HungerSystem's STARVING_THRESHOLD: the hunger a character who starved to death reached.
constexpr int STARVING_HUNGER = 900;
constexpr int FIRST_RUN_ROUNDS = 5;

// Every line the HUD's panel and the death screen read, joined part by part, and the
// line the screen shows as current.
std::vector<std::string> every_line_on_screen(const MessageSystem& messages)
{
	std::vector<std::string> lines{ messages.get_current_message() };
	for (size_t entry = 0; entry < messages.get_stored_message_count(); ++entry)
	{
		std::string line{};
		for (const LogMessage& part : messages.get_attack_message_at(entry))
		{
			line += part.text;
		}
		lines.push_back(line);
	}
	return lines;
}

bool is_any_line_holding(const std::vector<std::string>& lines, std::string_view text)
{
	for (const std::string& line : lines)
	{
		if (line.find(text) != std::string::npos)
		{
			return true;
		}
	}
	return false;
}
} // namespace

class NewRunStateTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
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

	// New Game, as the end of character creation calls it.
	void start_a_new_game()
	{
		game.playerBlueprint.creatureClass = CreatureClass::FIGHTER;
		game.playerBlueprint.playerRaceState = PlayerRaceState::HUMAN;
		game.stateManager.init_new_game(ctx);
	}

	// Rounds pass as the game loop feeds them: the clock moves, then the upkeep it owes runs.
	void let_rounds_pass(int rounds)
	{
		ctx.gameState->advance_clock(rounds * TIME_UNITS_PER_ROUND);
		ctx.gameLoopCoordinator->apply_round_upkeep(ctx);
	}

	// A run that went on for a while and starved: rounds run, hunger at Starving and told,
	// a finished line in the log, and a message still being written when it ended.
	void play_a_first_run()
	{
		start_a_new_game();
		let_rounds_pass(FIRST_RUN_ROUNDS);
		ctx.hungerSystem->increase_hunger(ctx, STARVING_HUNGER - ctx.hungerSystem->get_hunger_value());
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, FIRST_RUN_LINE, MessageCompletion::FINISHED);
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, FIRST_RUN_PART);
		ASSERT_EQ(ctx.gameState->get_rounds_run(), FIRST_RUN_ROUNDS) << "the first run's rounds did not run";
		ASSERT_EQ(ctx.hungerSystem->get_hunger_state(), HungerState::STARVING);
	}

	// Nothing of the first run's messages is on screen or waiting to join the next line.
	void expect_no_first_run_message()
	{
		// A finished line now closes whatever part was left waiting, so a leftover shows here.
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "second run line", MessageCompletion::FINISHED);
		const std::vector<std::string> lines = every_line_on_screen(*ctx.messageSystem);
		EXPECT_FALSE(is_any_line_holding(lines, FIRST_RUN_LINE)) << "the last run's line is still on screen";
		EXPECT_FALSE(is_any_line_holding(lines, FIRST_RUN_PART)) << "the last run's unfinished part joined a new line";
		EXPECT_FALSE(is_any_line_holding(lines, "Starving")) << "the last run's hunger notice is still on screen";
	}

	Game game{ TEST_DICE_SEED };
	GameContext ctx{ game.context() };
};

// The headline: the new game's first round is fed, which it was not while the round count
// stood at the last run's.
TEST_F(NewRunStateTest, ANewGamesFirstRoundIsFed)
{
	play_a_first_run();

	start_a_new_game();
	EXPECT_EQ(ctx.gameState->get_time(), 0);
	EXPECT_EQ(ctx.gameState->get_rounds_run(), 0) << "the round count carried over from the last run";
	const int hungerAtStart = ctx.hungerSystem->get_hunger_value();
	let_rounds_pass(1);

	EXPECT_EQ(ctx.gameState->get_rounds_run(), 1);
	EXPECT_EQ(ctx.hungerSystem->get_hunger_value(), hungerAtStart + 1) << "the first round of the new game fed no hunger";
}

// Hunger starts where a fresh game's does, and what the player was last told is forgotten:
// the first round of the new game reports no change of state.
TEST_F(NewRunStateTest, ANewGameStartsFresh)
{
	play_a_first_run();

	start_a_new_game();

	EXPECT_EQ(ctx.hungerSystem->get_hunger_value(), HungerSystem{}.get_hunger_value()) << "the last run's hunger carried over";
	let_rounds_pass(1);
	EXPECT_FALSE(is_any_line_holding(every_line_on_screen(*ctx.messageSystem), "You are now"))
		<< "the new game's first round reported a change from the last run's hunger";
}

TEST_F(NewRunStateTest, ANewGameShowsNothingOfTheLastRunsMessages)
{
	play_a_first_run();

	start_a_new_game();

	expect_no_first_run_message();
}

// Load Game starts from the same fresh state, and the save still decides the clock.
TEST_F(NewRunStateTest, ALoadedGameShowsNothingOfTheLastRunsMessagesAndKeepsItsClock)
{
	const std::filesystem::path savePath = Paths::resolve(Paths::SAVE_FILE);
	if (std::filesystem::exists(savePath))
	{
		GTEST_SKIP() << std::format("a save is already at {}, and this test will not overwrite it", savePath.string());
	}

	// The game being saved, then a further run on top of it before it is loaded.
	start_a_new_game();
	let_rounds_pass(FIRST_RUN_ROUNDS);
	const int savedTime = ctx.gameState->get_time();
	const int savedRounds = ctx.gameState->get_rounds_run();
	game.stateManager.save_game(ctx);
	let_rounds_pass(FIRST_RUN_ROUNDS);
	ctx.hungerSystem->increase_hunger(ctx, STARVING_HUNGER - ctx.hungerSystem->get_hunger_value());
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, FIRST_RUN_LINE, MessageCompletion::FINISHED);
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, FIRST_RUN_PART);

	const LoadOutcome outcome = game.stateManager.load_all(ctx);
	ASSERT_TRUE(GameStateManager::delete_save_file()) << "the stand-in save at " << savePath.string() << " was left behind";

	ASSERT_EQ(outcome.kind, LoadOutcomeKind::LOADED) << outcome.reason;
	EXPECT_EQ(ctx.gameState->get_time(), savedTime) << "the loaded game did not keep its saved clock";
	EXPECT_EQ(ctx.gameState->get_rounds_run(), savedRounds) << "the loaded game did not keep its saved round count";
	expect_no_first_run_message();
}

// end of file: NewRunStateTest.cpp
