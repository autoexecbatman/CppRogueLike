// Checks what stepping onto a trap does in each of its states, and that the damage it
// deals is the damage its dice describe.
//
// What it is for. A hidden trap gets one detection roll as a creature steps onto it.
// Missed, it springs; noticed, the creature stops short and it does not. A trap already
// known - found earlier, or left armed by a failed disarm - springs with no second
// roll, and a disarmed trap does nothing. Springing rolls every die: a 2d6 pit ranges
// from two to twelve.
//
// Every roll a test depends on is scripted, in the order the trap asks for it. A roll
// left unscripted is real, so a test that expects no damage leaves the damage dice
// unscripted on purpose: a trap that sprang anyway would roll at least two.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TrapDamageTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/TileFeature.h"
#include "src/Trap.h"
#include "tests/mocks/MockGameContext.h"

class TrapDamageTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		victim.healthPool = std::make_unique<HealthPool>(40);
		// Ten gives a dexterity modifier of zero, so each check is the bare roll:
		// 20 finds and disarms, 1 misses both.
		victim.set_dexterity(10);
	}

	// Walks the victim onto the pit once with a detection roll of 20, so the trap is
	// known and unsprung when the test proper begins.
	void notice(Trap& pit)
	{
		mock.dice.set_next_roll(20);
		pit.on_creature_enter(victim, ctx);
		ASSERT_EQ(pit.get_state(), TrapState::DETECTED) << "a detection roll of 20 finds the trap";
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

	// Detection 1 misses, so the pit springs; the two dice are six and six; the
	// destruction coin shows two, so the trap stays.
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(2);
	const int before = victim.get_hp();

	pit.on_creature_enter(victim, ctx);

	EXPECT_EQ(before - victim.get_hp(), 12) << "2d6 at six and six is twelve";
}

TEST_F(TrapDamageTest, AHiddenTrapYouMissSpringsAndStopsTheStep)
{
	Trap pit{ Vector2D{ 5, 5 }, TrapType::PIT, mock.tile_config };

	// Detection 1 misses; the dice are one and one; the coin keeps the trap.
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(2);
	const int before = victim.get_hp();

	const EntryResult entered = pit.on_creature_enter(victim, ctx);

	EXPECT_EQ(entered, EntryResult::BLOCKED) << "the victim is in the pit, not past it";
	EXPECT_EQ(pit.get_state(), TrapState::TRIGGERED);
	EXPECT_EQ(before - victim.get_hp(), 2) << "an unnoticed pit springs: 2d6 at one and one";
}

TEST_F(TrapDamageTest, AHiddenTrapNoticedAtTheLastMomentStopsTheStepUnsprung)
{
	Trap pit{ Vector2D{ 5, 5 }, TrapType::PIT, mock.tile_config };

	// Detection 20 finds it. Nothing after is scripted.
	mock.dice.set_next_roll(20);
	const int before = victim.get_hp();

	const EntryResult entered = pit.on_creature_enter(victim, ctx);

	EXPECT_EQ(entered, EntryResult::BLOCKED) << "the victim stops short of the pit";
	EXPECT_EQ(pit.get_state(), TrapState::DETECTED);
	EXPECT_EQ(victim.get_hp(), before) << "a trap seen in time is not stepped into";
}

TEST_F(TrapDamageTest, AKnownTrapSpringsWithoutAnotherDetectionRoll)
{
	Trap pit{ Vector2D{ 5, 5 }, TrapType::PIT, mock.tile_config };
	ASSERT_NO_FATAL_FAILURE(notice(pit));

	// Stepping onto the known pit: the dice are six and six and the coin keeps it. A
	// detection roll here would take the first six and the damage would not be twelve.
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(2);
	const int before = victim.get_hp();

	const EntryResult entered = pit.on_creature_enter(victim, ctx);

	EXPECT_EQ(entered, EntryResult::BLOCKED);
	EXPECT_EQ(before - victim.get_hp(), 12) << "walking onto a known trap sets it off";
}

TEST_F(TrapDamageTest, AFailedDisarmSpringsTheTrapOnTheDisarmer)
{
	Trap pit{ Vector2D{ 5, 5 }, TrapType::PIT, mock.tile_config };
	ASSERT_NO_FATAL_FAILURE(notice(pit));

	// Disarm 1 fails against twelve; the dice are six and six; the coin keeps it.
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(2);
	const int before = victim.get_hp();

	EXPECT_EQ(pit.attempt_disarm(victim, ctx), DisarmResult::TRIGGERED);
	EXPECT_EQ(before - victim.get_hp(), 12) << "the trap goes off on whoever was working on it";
}

TEST_F(TrapDamageTest, ADisarmedTrapIsInert)
{
	Trap pit{ Vector2D{ 5, 5 }, TrapType::PIT, mock.tile_config };
	ASSERT_NO_FATAL_FAILURE(notice(pit));

	// Disarm 20 succeeds. Nothing after is scripted.
	mock.dice.set_next_roll(20);
	ASSERT_EQ(pit.attempt_disarm(victim, ctx), DisarmResult::DISARMED);
	const int before = victim.get_hp();

	EXPECT_EQ(pit.on_creature_enter(victim, ctx), EntryResult::UNAFFECTED);
	EXPECT_EQ(victim.get_hp(), before) << "a disarmed trap is walked over";
}
