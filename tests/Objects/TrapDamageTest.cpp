// Checks what stepping onto a trap does in each of its states, and that the damage it
// deals is the damage its dice describe.
//
// What it is for. Stepping onto a trap springs it, hidden or known, with no roll to
// notice it: only a search finds a trap. A disarmed trap does nothing. Springing rolls
// every die: a 2d6 pit ranges from two to twelve.
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

#include "src/Colors.h"
#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/Player.h"
#include "src/PlayerRaceState.h"
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
		victim.set_creature_level(1);
	}

	// Finds the pit the way the game does, with a dwarf's search rolling a 1 on 1d6, so
	// the trap is known and unsprung when the test proper begins.
	// It answers what the search said; the caller asserts FOUND, so a search
	// that did not roll stops the test before its 1 reaches another die.
	[[nodiscard]] TrapSearchResult find(Trap& pit)
	{
		victim.lifespan->race = PlayerRaceState::DWARF;
		mock.dice.set_next_roll(1);
		return pit.search(victim, ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
	// The player, since a trap the player walks onto is the case a notice roll was for.
	Player victim{ Vector2D{ 5, 5 } };
};

TEST_F(TrapDamageTest, APitDealsTwoSixSidedDice)
{
	Trap pit{ Vector2D{ 5, 5 }, TrapType::PIT, mock.tile_config };
	ASSERT_EQ(pit.get_damage_dice_count(), 2);
	ASSERT_EQ(pit.get_damage_dice_size(), 6);

	// The two dice are six and six; the destruction coin shows two, so the trap stays.
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(2);
	const int before = victim.get_hp();

	pit.on_creature_enter(victim, ctx);

	EXPECT_EQ(before - victim.get_hp(), 12) << "2d6 at six and six is twelve";
}

TEST_F(TrapDamageTest, AHiddenTrapSpringsOnThePlayerWithNoRollToNoticeIt)
{
	Trap pit{ Vector2D{ 5, 5 }, TrapType::PIT, mock.tile_config };

	// The dice are six and six; the coin keeps the trap. A roll to notice it would take
	// the first six, and the damage would not be twelve.
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(2);
	const int before = victim.get_hp();

	const EntryResult entered = pit.on_creature_enter(victim, ctx);

	EXPECT_EQ(entered, EntryResult::BLOCKED)
		<< "the victim is in the pit, not past it";
	EXPECT_EQ(pit.get_state(), TrapState::TRIGGERED);
	EXPECT_EQ(before - victim.get_hp(), 12)
		<< "a hidden pit springs: 2d6 at six and six";
}

TEST_F(TrapDamageTest, AKnownTrapSpringsWithoutARollToNoticeIt)
{
	Trap pit{ Vector2D{ 5, 5 }, TrapType::PIT, mock.tile_config };
	ASSERT_EQ(find(pit), TrapSearchResult::FOUND);

	// Stepping onto the known pit: the dice are six and six and the coin keeps it. A
	// roll to notice it here would take the first six and the damage would not be twelve.
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(2);
	const int before = victim.get_hp();

	const EntryResult entered = pit.on_creature_enter(victim, ctx);

	EXPECT_EQ(entered, EntryResult::BLOCKED);
	EXPECT_EQ(before - victim.get_hp(), 12)
		<< "walking onto a known trap sets it off";
}
