// file: HungerSystemTest.cpp
//
// hungerValue is the only fact HungerSystem stores about how fed a creature is.
// Everything else follows from it, and these pin each of the three things that
// do: the fullness a meter draws, the state a label names, and the message that
// fires when the counter crosses a threshold.
//
// The counter runs the opposite way to all of them - 0 is a full stomach and
// hungerMax is dead - and both defects this file was written for came from that.
// The HUD drew hungerValue/hungerMax as fullness, so a well fed creature showed
// an empty bar; and the state was a stored copy that started out disagreeing with
// the counter, so turn one read "Satiated" over a full bar and logged a state
// change that had not happened.
//
// Run it:
//
//   cmake --build build --config Debug --target test_exe
//   cd build/bin/Debug && ./test_exe.exe --gtest_filter=HungerSystemTest.*

#include <array>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "src/GameContext.h"
#include "src/HungerSystem.h"
#include "tests/mocks/MockGameContext.h"

class HungerSystemTest : public ::testing::Test
{
protected:
	MockGameContext mock{};
	GameContext ctx{};
	HungerSystem hunger{};

	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.hungerSystem = &hunger;
	}
};

// A creature that has just eaten is full, so the meter is full.
TEST_F(HungerSystemTest, AFedCreatureIsCompletelyFull)
{
	EXPECT_FLOAT_EQ(hunger.get_fullness_ratio(), 1.0f);
}

// Hunger rising empties the meter rather than filling it. This is the direction
// the HUD had backwards.
TEST_F(HungerSystemTest, RisingHungerEmptiesTheMeter)
{
	const float wellFed = hunger.get_fullness_ratio();
	hunger.increase_hunger(ctx, hunger.get_hunger_max() / 4);

	EXPECT_LT(hunger.get_fullness_ratio(), wellFed);
	EXPECT_FLOAT_EQ(hunger.get_fullness_ratio(), 0.75f);
}

// Starvation is an empty meter, not a negative one.
TEST_F(HungerSystemTest, StarvationIsEmptyAndStopsThere)
{
	const int hungerMax = hunger.get_hunger_max();
	hunger.increase_hunger(ctx, hungerMax * 2);

	// The counter itself stops, which the ratio alone cannot show: get_fullness_ratio
	// clamps on its own, so it reads 0.0 for a counter that ran past the maximum.
	EXPECT_EQ(hunger.get_hunger_value(), hungerMax);
	EXPECT_FLOAT_EQ(hunger.get_fullness_ratio(), 0.0f);
}

// Eating more than you are hungry for leaves a full stomach rather than a
// negative one. The same blind spot as the ceiling, at the other end.
TEST_F(HungerSystemTest, EatingPastFullStopsAtZero)
{
	hunger.increase_hunger(ctx, 100);
	ASSERT_EQ(hunger.get_hunger_value(), 100);

	hunger.decrease_hunger(ctx, 500);

	EXPECT_EQ(hunger.get_hunger_value(), 0);
	EXPECT_FLOAT_EQ(hunger.get_fullness_ratio(), 1.0f);
}

// A new game has not moved between states, so turn one has nothing to announce.
// The stored state cache made oldState disagree with the counter, and every new
// game logged "You are now Well Fed." with the counter at 1.
TEST_F(HungerSystemTest, TheFirstTickAnnouncesNothing)
{
	ASSERT_EQ(mock.messages.get_stored_message_count(), 0u);

	hunger.increase_hunger(ctx, 1);

	EXPECT_EQ(mock.messages.get_stored_message_count(), 0u);
}

// The other half of that claim: silence on turn one must not come from a notifier
// that never speaks. Crossing a threshold is a real transition and still reaches
// the player.
TEST_F(HungerSystemTest, CrossingAThresholdAnnouncesTheNewState)
{
	// One ordinary turn, which establishes the baseline without a message.
	hunger.increase_hunger(ctx, 1);
	ASSERT_EQ(mock.messages.get_stored_message_count(), 0u);

	// Past WELL_FED_THRESHOLD at 200, so the counter lands in SATIATED.
	hunger.increase_hunger(ctx, 200);

	ASSERT_EQ(hunger.get_hunger_state(), HungerState::SATIATED);
	EXPECT_EQ(mock.messages.get_stored_message_count(), 1u);
}

// The threshold table calls a counter of 0 WELL_FED, so that is what a system
// nobody has ticked yet has to report. It said "Satiated" over a full bar.
TEST_F(HungerSystemTest, AFreshSystemReportsTheStateItsCounterImplies)
{
	EXPECT_EQ(hunger.get_hunger_value(), 0);
	EXPECT_EQ(hunger.get_hunger_state(), HungerState::WELL_FED);
	EXPECT_EQ(hunger.get_hunger_state_string(), "Well Fed");
}

// The threshold table, restated from the contract rather than from the code: the
// state is whatever the counter says it is, at the boundary and one past it.
TEST_F(HungerSystemTest, StateFollowsTheCounterAcrossEveryThreshold)
{
	struct Boundary
	{
		int hungerValue;
		HungerState expected;
	};

	const std::array<Boundary, 10> boundaries{ {
		{ 200, HungerState::WELL_FED },
		{ 201, HungerState::SATIATED },
		{ 400, HungerState::SATIATED },
		{ 401, HungerState::HUNGRY },
		{ 700, HungerState::HUNGRY },
		{ 701, HungerState::STARVING },
		{ 900, HungerState::STARVING },
		{ 901, HungerState::DYING },
		{ 1000, HungerState::DYING },
		{ 0, HungerState::WELL_FED },
	} };

	for (const Boundary& boundary : boundaries)
	{
		// Drive the counter to the exact value from wherever it currently sits.
		const int current = hunger.get_hunger_value();
		if (boundary.hungerValue >= current)
		{
			hunger.increase_hunger(ctx, boundary.hungerValue - current);
		}
		else
		{
			hunger.decrease_hunger(ctx, current - boundary.hungerValue);
		}

		EXPECT_EQ(hunger.get_hunger_value(), boundary.hungerValue);
		EXPECT_EQ(hunger.get_hunger_state(), boundary.expected)
			<< "hungerValue " << boundary.hungerValue;
	}
}

// A creature that has eaten is not suffering, and one that is hungry or worse is.
// The boundary is the HUNGRY threshold, so this is the case that can see it move.
TEST_F(HungerSystemTest, PenaltiesBeginExactlyWhereHungerDoes)
{
	hunger.increase_hunger(ctx, 400);
	ASSERT_EQ(hunger.get_hunger_state(), HungerState::SATIATED);
	EXPECT_FALSE(hunger.is_suffering_hunger_penalties()) << "a satiated creature was reported as suffering";

	hunger.increase_hunger(ctx, 1);
	ASSERT_EQ(hunger.get_hunger_state(), HungerState::HUNGRY);
	EXPECT_TRUE(hunger.is_suffering_hunger_penalties()) << "hunger arrived without its penalties";
}

// What the counter is, and what the player was last told, both survive a save. The
// second is the one worth pinning: it is the only thing stopping a load announcing
// a state change the player has already been told about.
TEST_F(HungerSystemTest, ACounterAndWhatWasAnnouncedBothSurviveASave)
{
	hunger.increase_hunger(ctx, 1);

	// One call from WELL_FED to HUNGRY passes SATIATED without stopping, and the
	// player is told what they are now rather than everything they went through.
	hunger.increase_hunger(ctx, 500);
	ASSERT_EQ(hunger.get_hunger_state(), HungerState::HUNGRY);
	ASSERT_EQ(mock.messages.get_stored_message_count(), 1u);

	nlohmann::json saved;
	hunger.save(saved);

	HungerSystem loaded{};
	loaded.load(saved);

	EXPECT_EQ(loaded.get_hunger_value(), hunger.get_hunger_value());
	EXPECT_EQ(loaded.get_hunger_state(), hunger.get_hunger_state());

	// The load restored what was announced, so an unchanged state says nothing more.
	const size_t before = mock.messages.get_stored_message_count();
	loaded.increase_hunger(ctx, 1);
	EXPECT_EQ(mock.messages.get_stored_message_count(), before)
		<< "the loaded system re-announced a state the player already knew";
}

// A load that forgets what was announced is silent exactly once, and it is the
// once that matters: the first threshold the player crosses afterwards. An
// ordinary tick after loading cannot show this, because an empty record and a
// matching record are both silent.
TEST_F(HungerSystemTest, AThresholdCrossedRightAfterLoadingIsStillAnnounced)
{
	hunger.increase_hunger(ctx, 1);
	hunger.increase_hunger(ctx, 500); // into HUNGRY, announced
	ASSERT_EQ(hunger.get_hunger_state(), HungerState::HUNGRY);

	nlohmann::json saved;
	hunger.save(saved);

	HungerSystem loaded{};
	loaded.load(saved);

	const size_t before = mock.messages.get_stored_message_count();

	// The first thing that happens after the load crosses into STARVING.
	loaded.increase_hunger(ctx, 250);

	ASSERT_EQ(loaded.get_hunger_state(), HungerState::STARVING);
	EXPECT_EQ(mock.messages.get_stored_message_count(), before + 1)
		<< "the load lost what was announced, so the first crossing after it went unsaid";
}
