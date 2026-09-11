// file: HungerFullnessTest.cpp
//
// hungerValue counts up toward starvation: 0 is a full stomach and hungerMax is
// dead. Anything that draws a meter wants the opposite of that, so
// get_fullness_ratio is the complement, and these pin the direction.
//
// The HUD drew hungerValue/hungerMax directly before this existed, so a well fed
// creature showed an empty bar beside a health bar that fills when healthy.
//
// Run it:
//
//   cmake --build build --config Debug --target test_exe
//   cd build/bin/Debug && ./test_exe.exe --gtest_filter=HungerFullnessTest.*

#include <gtest/gtest.h>

#include "src/Core/GameContext.h"
#include "src/Systems/HungerSystem.h"
#include "tests/mocks/MockGameContext.h"

class HungerFullnessTest : public ::testing::Test
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
TEST_F(HungerFullnessTest, AFedCreatureIsCompletelyFull)
{
	EXPECT_FLOAT_EQ(hunger.get_fullness_ratio(), 1.0f);
}

// Hunger rising empties the meter rather than filling it. This is the direction
// the HUD had backwards.
TEST_F(HungerFullnessTest, RisingHungerEmptiesTheMeter)
{
	const float wellFed = hunger.get_fullness_ratio();
	hunger.increase_hunger(ctx, hunger.get_hunger_max() / 4);

	EXPECT_LT(hunger.get_fullness_ratio(), wellFed);
	EXPECT_FLOAT_EQ(hunger.get_fullness_ratio(), 0.75f);
}

// Starvation is an empty meter, not a negative one.
TEST_F(HungerFullnessTest, StarvationIsEmptyAndStopsThere)
{
	hunger.increase_hunger(ctx, hunger.get_hunger_max() * 2);

	EXPECT_FLOAT_EQ(hunger.get_fullness_ratio(), 0.0f);
}

// NOT TESTED HERE, and it is a live defect: a fresh HungerSystem reports
// "Satiated" while its own counter is 0, which the threshold table calls WELL_FED.
// currentState is initialised independently of hungerValue, so the two disagree
// until the first tick corrects the cache. Reported rather than fixed; a test
// asserting the contract fails today.
