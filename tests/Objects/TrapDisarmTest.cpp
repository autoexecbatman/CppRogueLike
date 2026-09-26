// file: TrapDisarmTest.cpp
// Working on a trap, by the Player's Handbook's Find/Remove Traps (PDF page 87).
//
// The book's whole rule for the attempt: "Once a trap is found, the thief can try to
// remove it or disarm it. This also requires 1d10 rounds. If the dice roll indicates
// success, the trap is disarmed. If the dice roll indicates failure, the trap is beyond
// the thief's current skill. He can try disarming the trap again when he advances to
// the next experience level. If the dice roll is 96-100, the thief accidentally
// triggers the trap and suffers the consequences."
//
// Three claims follow from that paragraph and none of them was true of the d20 against
// a hard-coded DC 12 this replaced: the number rolled against is the character's own
// percentage, an ordinary failure leaves the trap alone, and a character who has no
// such percentage cannot try at all.
//
// Every roll is scripted in the order the trap asks for it. A roll left unscripted is
// real, so a test that expects no damage leaves the damage dice unscripted on purpose:
// a trap that sprang anyway would roll at least two.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TrapDisarmTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/Player.h"
#include "src/ThiefSkills.h"
#include "src/TileFeature.h"
#include "src/Trap.h"
#include "tests/mocks/MockGameContext.h"

class TrapDisarmTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &thief;

		thief->healthPool = std::make_unique<HealthPool>(40);
		thief->set_creature_class(CreatureClass::ROGUE);
		thief->playerClassState = Player::PlayerClassState::ROGUE;
		// Dexterity 13 is the middle of Table 28's flat row, so the percentage below
		// is the base score and the points, with nothing else folded in.
		thief->set_dexterity(13);
		thief->playerRaceState = Player::PlayerRaceState::HUMAN;
		thief->set_creature_level(1);
	}

	// Puts a known, unsprung trap of that kind in front of the thief.
	std::unique_ptr<Trap> found_trap(TrapType kind)
	{
		auto trap = std::make_unique<Trap>(Vector2D{ 5, 5 }, kind, mock.tile_config);
		mock.dice.set_next_roll(20);
		trap->on_creature_enter(*thief, ctx);
		EXPECT_EQ(trap->get_state(), TrapState::DETECTED) << "a detection roll of 20 finds the trap";
		return trap;
	}

	// The dart trap the cases that do not care about damage use.
	std::unique_ptr<Trap> found_trap()
	{
		return found_trap(TrapType::DART);
	}

	// The 2d6 pit, where the damage a sprung trap deals is worth asserting.
	std::unique_ptr<Trap> found_pit()
	{
		return found_trap(TrapType::PIT);
	}

	// Find/Remove Traps is base 5, so this is what the character rolls against.
	void buy_find_remove_traps(int points)
	{
		thief->thiefSkillPoints.at(thief_skill_index(ThiefSkill::FIND_REMOVE_TRAPS)) = points;
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> thief{ std::make_unique<Player>(Vector2D{ 5, 5 }) };
};

// "If the dice roll indicates success, the trap is disarmed." The number rolled
// against is the character's Find/Remove Traps, which nothing else here can produce:
// base 5 plus 40 bought is 45, and a 45 succeeds where a 46 does not.
TEST_F(TrapDisarmTest, TheRollIsAgainstTheCharactersOwnPercentage)
{
	buy_find_remove_traps(40);
	ASSERT_EQ(thief->thief_skill(ThiefSkill::FIND_REMOVE_TRAPS), 45);

	auto onEdge = found_trap();
	mock.dice.set_next_roll(45);
	EXPECT_EQ(onEdge->attempt_disarm(*thief, ctx), DisarmResult::DISARMED);
	EXPECT_EQ(onEdge->get_state(), TrapState::DISARMED);

	auto justOver = found_trap();
	mock.dice.set_next_roll(46);
	EXPECT_EQ(justOver->attempt_disarm(*thief, ctx), DisarmResult::BEYOND_SKILL);
}

// "If the dice roll indicates failure, the trap is beyond the thief's current skill."
// It is not sprung - the d20 check this replaced set it off on every miss.
TEST_F(TrapDisarmTest, AnOrdinaryFailureLeavesTheTrapArmedAndTheThiefUnhurt)
{
	buy_find_remove_traps(20);
	auto pit = found_trap();
	const int before = thief->get_hp();

	// 90 is a failure and is below the 96 that springs it. The damage dice are left
	// unscripted: a trap that went off anyway would roll them.
	mock.dice.set_next_roll(90);

	EXPECT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::BEYOND_SKILL);
	EXPECT_EQ(pit->get_state(), TrapState::DETECTED) << "the trap is still there, still armed";
	EXPECT_EQ(thief->get_hp(), before) << "an ordinary failure costs nothing";
}

// "If the dice roll is 96-100, the thief accidentally triggers the trap and suffers
// the consequences."
TEST_F(TrapDisarmTest, NinetySixSpringsTheTrapOnTheThief)
{
	buy_find_remove_traps(20);
	auto pit = found_pit();
	const int before = thief->get_hp();

	// 96 springs it; the pit's two dice are six and six; the coin keeps the trap.
	mock.dice.set_next_roll(96);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(2);

	EXPECT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::TRIGGERED);
	EXPECT_EQ(before - thief->get_hp(), 12) << "2d6 at six and six, on whoever was working on it";
}

// 95 is the last failure that is only a failure, so the band is 96 to 100 rather
// than "the top five somewhere".
TEST_F(TrapDisarmTest, NinetyFiveIsStillOnlyAFailure)
{
	buy_find_remove_traps(20);
	auto pit = found_trap();
	const int before = thief->get_hp();

	mock.dice.set_next_roll(95);

	EXPECT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::BEYOND_SKILL);
	EXPECT_EQ(thief->get_hp(), before);
}

// "He can try disarming the trap again when he advances to the next experience level."
TEST_F(TrapDisarmTest, ASecondAttemptWaitsForTheNextLevel)
{
	buy_find_remove_traps(20);
	auto pit = found_trap();

	mock.dice.set_next_roll(90);
	ASSERT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::BEYOND_SKILL);

	// A certain success is queued and must not be reached: the attempt for this
	// level is spent. An unscripted roll here would be a real one, so the queue is
	// what makes the refusal visible rather than lucky.
	mock.dice.set_next_roll(1);
	EXPECT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::BEYOND_SKILL);

	// The level buys the next attempt, and the 1 is still waiting to be spent.
	thief->set_creature_level(2);
	EXPECT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::DISARMED)
		<< "a level gained is what buys the next attempt";
}

// The lockout is on that one trap, not on the character.
TEST_F(TrapDisarmTest, AFailureOnOneTrapDoesNotLockAnother)
{
	buy_find_remove_traps(20);
	auto failed = found_trap();
	mock.dice.set_next_roll(90);
	ASSERT_EQ(failed->attempt_disarm(*thief, ctx), DisarmResult::BEYOND_SKILL);

	auto another = found_trap();
	mock.dice.set_next_roll(1);
	EXPECT_EQ(another->attempt_disarm(*thief, ctx), DisarmResult::DISARMED);
}

// Find/Remove Traps is the thief's, and the book gives no one else a way to work on a
// trap. A fighter standing over one is told so rather than rolling for it.
TEST_F(TrapDisarmTest, AClassWithNoThiefSkillCannotTryAtAll)
{
	thief->set_creature_class(CreatureClass::FIGHTER);
	thief->playerClassState = Player::PlayerClassState::FIGHTER;
	auto pit = found_trap();

	// A certain success is queued and must not be reached.
	mock.dice.set_next_roll(1);
	EXPECT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::NO_SKILL);
	EXPECT_EQ(pit->get_state(), TrapState::DETECTED);
}

// A rogue whose adjustments leave the skill at zero has not bought it up to a usable
// percentage, which the book treats as not having it.
TEST_F(TrapDisarmTest, ARogueWhoHasNotBoughtTheSkillUpCannotTryEither)
{
	// Base 5, and plate mail has no Table 29 column at all, so there is no number.
	thief->set_dexterity(9);
	buy_find_remove_traps(0);
	auto pit = found_trap();

	// Base 5, Dexterity 9 is -10: the score is nothing.
	ASSERT_EQ(thief->thief_skill(ThiefSkill::FIND_REMOVE_TRAPS), 0);

	// A certain success is queued and must not be reached.
	mock.dice.set_next_roll(1);
	EXPECT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::NO_SKILL);
	EXPECT_EQ(pit->get_state(), TrapState::DETECTED);
}

// The states that answer before any roll are unchanged by the new rule.
TEST_F(TrapDisarmTest, AHiddenTrapAndADisarmedOneAnswerBeforeTheDice)
{
	buy_find_remove_traps(20);

	Trap hidden{ Vector2D{ 5, 5 }, TrapType::DART, mock.tile_config };
	EXPECT_EQ(hidden.attempt_disarm(*thief, ctx), DisarmResult::NOT_VISIBLE);

	auto done = found_pit();
	mock.dice.set_next_roll(1);
	ASSERT_EQ(done->attempt_disarm(*thief, ctx), DisarmResult::DISARMED);
	EXPECT_EQ(done->attempt_disarm(*thief, ctx), DisarmResult::ALREADY_DISARMED);

	// And it is inert underfoot: nothing after the disarm is scripted, so a trap
	// that went off would roll real dice and take real hit points.
	const int before = thief->get_hp();
	EXPECT_EQ(done->on_creature_enter(*thief, ctx), EntryResult::UNAFFECTED);
	EXPECT_EQ(thief->get_hp(), before) << "a disarmed trap is walked over";
}

// end of file: TrapDisarmTest.cpp
