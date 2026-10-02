// file: CreatureScheduleTest.cpp
// Where a creature stands in the turn order, and what arriving on a level does to it.
//
// The owner's requirement for the pace system was that it work in both directions:
// "a player moves, the snail skips, and the quickling moves and attacks multiple
// times." Both come out of one expression here, and nothing in the code branches on
// whether a creature is fast or slow.
//
// The second half is the funnel. A creature pushed onto the level without being
// placed on the clock stands at time zero, and a level generated at round 400 would
// owe it four hundred actions at once. add_creature is the one way in.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CreatureScheduleTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/Ai.h"
#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/CreatureManager.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/TurnSchedule.h"
#include "tests/mocks/CountingAi.h"
#include "tests/mocks/MockGameContext.h"

class CreatureScheduleTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.creatures = &creatures;
		ctx.creatureManager = &manager;
	}

	std::unique_ptr<Creature> built(int actionDelay)
	{
		auto creature = std::make_unique<Creature>(
			Vector2D{ 5, 5 },
			ActorData{ TileRef{}, "subject", ColorPairId::WHITE_BLACK });
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creature->armorClass = std::make_unique<ArmorClass>(10);
		creature->healthPool = std::make_unique<HealthPool>(10);
		creature->set_action_delay(actionDelay);
		return creature;
	}

	MockGameContext mock{};
	GameContext ctx{};
	CreatureManager manager{};
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// A creature's ordinary action costs exactly its action delay, with nothing converted
// on the way. That is the whole of what collapsing the percentage bought: the stored
// number is the cost, so there is no second answer to the same question.
TEST_F(CreatureScheduleTest, AnOrdinaryActionCostsTheCreaturesOwnDelay)
{
	EXPECT_EQ(built(TIME_UNITS_PER_ROUND)->get_action_delay(), TIME_UNITS_PER_ROUND);
	EXPECT_EQ(built(TIME_UNITS_PER_ROUND / 2)->get_action_delay(), TIME_UNITS_PER_ROUND / 2);
	EXPECT_EQ(built(TIME_UNITS_PER_ROUND * 2)->get_action_delay(), TIME_UNITS_PER_ROUND * 2);
}

// One action per round, which is every creature in the game until something sets a
// delay of its own, and is why the suite stayed green when the schedule was wired in.
TEST_F(CreatureScheduleTest, AnOrdinaryCreatureIsDueOneActionPerRound)
{
	const auto ordinary = built(TIME_UNITS_PER_ROUND);

	EXPECT_EQ(ordinary->scheduled_actions_before(TIME_UNITS_PER_ROUND), 1);
	EXPECT_EQ(ordinary->scheduled_actions_before(TIME_UNITS_PER_ROUND * 3), 3);
}

// The quickling, by name. Half the delay is two actions in the window the player's one
// action opens, and it spends them however its Ai decides.
TEST_F(CreatureScheduleTest, AQuicklingIsDueSeveralActionsInOneRound)
{
	const auto quickling = built(TIME_UNITS_PER_ROUND / 2);
	EXPECT_EQ(quickling->scheduled_actions_before(TIME_UNITS_PER_ROUND), 2);

	const auto blur = built(TIME_UNITS_PER_ROUND / 4);
	EXPECT_EQ(blur->scheduled_actions_before(TIME_UNITS_PER_ROUND), 4);
}

// The snail, by name. It acts, overshoots, and the next window holds nothing for it -
// a skipped turn with nothing in the code saying "skip".
TEST_F(CreatureScheduleTest, ASnailSkipsTheWindowItHasOvershot)
{
	auto snail = built(TIME_UNITS_PER_ROUND * 2);

	EXPECT_EQ(snail->scheduled_actions_before(TIME_UNITS_PER_ROUND), 1);

	// It acted, which puts it two rounds out.
	snail->set_next_action_time(snail->get_action_delay());

	EXPECT_EQ(snail->scheduled_actions_before(TIME_UNITS_PER_ROUND * 2), 0) << "the snail acted twice";
	EXPECT_EQ(snail->scheduled_actions_before(TIME_UNITS_PER_ROUND * 3), 1);
}

// A creature arrives standing where the clock stands, so it is due nothing until a
// round has passed - the same as every creature already on the level.
TEST_F(CreatureScheduleTest, AnArrivalStandsWhereTheClockStands)
{
	ctx.gameState->set_time(TIME_UNITS_PER_ROUND * 7);

	manager.add_creature(built(TIME_UNITS_PER_ROUND), ctx);

	ASSERT_EQ(creatures.size(), 1u);
	EXPECT_EQ(creatures.front()->get_next_action_time(), TIME_UNITS_PER_ROUND * 7);
	EXPECT_EQ(creatures.front()->scheduled_actions_before(ctx.gameState->get_time()), 0)
		<< "a creature that just arrived acted before anything happened";
}

// The regression the funnel exists for. A creature left standing at time zero on a
// level generated hours in is owed every action since the game began, which would
// empty a dungeon into the player's face in one frame.
TEST_F(CreatureScheduleTest, AnArrivalIsNotOwedEveryActionSinceTheGameBegan)
{
	ctx.gameState->set_time(TIME_UNITS_PER_ROUND * 400);

	manager.add_creature(built(TIME_UNITS_PER_ROUND), ctx);
	const int placed = creatures.front()->scheduled_actions_before(ctx.gameState->get_time());

	// What it would have been without the placement, for the contrast to be visible.
	auto unplaced = built(TIME_UNITS_PER_ROUND);
	const int backlog = unplaced->scheduled_actions_before(ctx.gameState->get_time());

	EXPECT_EQ(placed, 0);
	EXPECT_EQ(backlog, 400) << "the arithmetic no longer owes an unplaced creature a backlog";
}

// Driven for real, through the public loop, with a mind that counts. This is the
// owner's requirement end to end: one round of the clock, and the ordinary creature
// acts once, the quickling twice, the snail once and then not at all.
TEST_F(CreatureScheduleTest, OneRoundDrivesEachCreatureAsOftenAsItsSpeedAllows)
{
	auto ordinary = built(TIME_UNITS_PER_ROUND);
	auto quickling = built(TIME_UNITS_PER_ROUND / 2);
	auto snail = built(TIME_UNITS_PER_ROUND * 2);

	auto* ordinaryMind = new CountingAi{};
	auto* quicklingMind = new CountingAi{};
	auto* snailMind = new CountingAi{};
	ordinary->ai.reset(ordinaryMind);
	quickling->ai.reset(quicklingMind);
	snail->ai.reset(snailMind);

	creatures.push_back(std::move(ordinary));
	creatures.push_back(std::move(quickling));
	creatures.push_back(std::move(snail));

	// One round of the clock has passed.
	ctx.gameState->set_time(TIME_UNITS_PER_ROUND);
	manager.update_creatures(creatures, ctx);

	EXPECT_EQ(ordinaryMind->updates, 1);
	EXPECT_EQ(quicklingMind->updates, 2) << "the quickling did not act twice";
	EXPECT_EQ(snailMind->updates, 1);

	// A second round. The snail overshot on the first and skips this one.
	ctx.gameState->set_time(TIME_UNITS_PER_ROUND * 2);
	manager.update_creatures(creatures, ctx);

	EXPECT_EQ(ordinaryMind->updates, 2);
	EXPECT_EQ(quicklingMind->updates, 4);
	EXPECT_EQ(snailMind->updates, 1) << "the snail did not skip";
}

// Over ten rounds the counts are exactly the speeds, which is the property a single
// window cannot show: a creature that drifts by one action a round looks right once.
TEST_F(CreatureScheduleTest, TheCountsDoNotDriftOverTenRounds)
{
	auto quickling = built(TIME_UNITS_PER_ROUND / 2);
	auto* mind = new CountingAi{};
	quickling->ai.reset(mind);
	creatures.push_back(std::move(quickling));

	for (int round = 1; round <= 10; ++round)
	{
		ctx.gameState->set_time(TIME_UNITS_PER_ROUND * round);
		manager.update_creatures(creatures, ctx);
	}

	EXPECT_EQ(mind->updates, 20);
}
