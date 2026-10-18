// file: RunOutcomeTest.cpp
// How a run ends, and what may undo it. The outcome lives apart from the turn's phase,
// so moving the turn on cannot erase a win or a loss; only the start of a new run
// forgets one. PlayerDeathTest covers the losing half through Player::die.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RunOutcomeTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/ArmorClass.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/PlayerTurn.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

class RunOutcomeTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;

		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 1, 1 }) };
};

// Using the Amulet of Yendor is the win.
TEST_F(RunOutcomeTest, UsingTheAmuletOfYendorWinsTheRun)
{
	auto amulet = ItemCreator::create("amulet_of_yendor", Vector2D{ 1, 1 }, ctx);
	[[maybe_unused]] const bool spent = use_item(*amulet->behavior, *amulet, *player, ctx);

	EXPECT_EQ(ctx.gameState->get_run_outcome(), RunOutcome::VICTORY)
		<< "the amulet was used and the run was not won";
}

// A won run is as final as a lost one: an action spent after the win does not undo it.
TEST_F(RunOutcomeTest, SpendingAnActionAfterTheWinKeepsTheVictory)
{
	ctx.gameState->set_run_outcome(RunOutcome::VICTORY);
	spend_player_action(ctx, TIME_UNITS_PER_ROUND);

	EXPECT_EQ(ctx.gameState->get_run_outcome(), RunOutcome::VICTORY)
		<< "spending an action erased the victory, so no victory screen opens";
}

// A new run, started or loaded, forgets how the last one ended and starts its turn
// from the beginning.
TEST_F(RunOutcomeTest, BeginningARunForgetsTheLastOutcome)
{
	ctx.gameState->set_run_outcome(RunOutcome::DEFEAT);
	ctx.gameState->set_game_status(GameStatus::NEW_TURN);

	ctx.gameState->begin_run();

	EXPECT_EQ(ctx.gameState->get_run_outcome(), RunOutcome::UNDECIDED)
		<< "a new run still carries the last run's defeat, so it opens on the death screen";
	EXPECT_EQ(ctx.gameState->get_game_status(), GameStatus::STARTUP)
		<< "a new run did not start its turn from the beginning";
}

// end of file: RunOutcomeTest.cpp
