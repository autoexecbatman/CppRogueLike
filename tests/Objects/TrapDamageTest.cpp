// Checks that a trap deals the damage its dice describe.
//
// What it is for. A pit trap is authored as 2d6, and Trap::roll_damage rolled
// dice.roll(count, size) - a single draw between two and six, as though the dice count
// were the minimum and the die size the maximum. A pit could never deal more than six.
// Every damage roll now goes through roll_dice, the one function that turns a dice
// expression into a number.
//
// A trap here only springs once it is detected - see the rolls below. That ordering is
// its own open question and is not what this file checks.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TrapDamageTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/Trap.h"
#include "tests/mocks/MockGameContext.h"

class TrapDamageTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		victim.healthPool = std::make_unique<HealthPool>(40);
		// Ten gives a dexterity modifier of zero, so the detection roll is the roll.
		victim.set_dexterity(10);
	}

	MockGameContext mock{};
	GameContext ctx{};
	Creature victim{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "adventurer", 0 } };
};

TEST_F(TrapDamageTest, APitDealsTwoSixSidedDice)
{
	Trap pit{ Vector2D{ 5, 5 }, TrapType::PIT, mock.tile_config };
	ASSERT_EQ(pit.get_damage_dice_count(), 2);
	ASSERT_EQ(pit.get_damage_dice_size(), 6);

	// Detection 20 springs it; the two dice are six and six; the destruction coin
	// shows two, so the trap stays.
	mock.dice.set_next_roll(20);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(2);
	const int before = victim.get_hp();

	pit.on_creature_enter(victim, ctx);

	EXPECT_EQ(before - victim.get_hp(), 12) << "2d6 at six and six is twelve";
}
