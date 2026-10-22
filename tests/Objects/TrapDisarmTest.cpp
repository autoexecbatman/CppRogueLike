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
#include <optional>
#include <string>

#include "src/GameContext.h"
#include "src/GameStateManager.h"
#include "src/HealthPool.h"
#include "src/MessageSystem.h"
#include "src/Player.h"
#include "src/PlayerRaceState.h"
#include "src/ThiefSkills.h"
#include "src/TileFeature.h"
#include "src/Trap.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
const Vector2D EAST_OF_THIEF{ 6, 5 };
const Vector2D WEST_OF_THIEF{ 4, 5 };
} // namespace

class TrapDisarmTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &thief;

		thief->healthPool = std::make_unique<HealthPool>(40);
		thief->set_creature_class(CreatureClass::ROGUE);
		// Dexterity 13 is the middle of Table 28's flat row, so the percentage below
		// is the base score and the points, with nothing else folded in.
		thief->set_dexterity(13);
		thief->lifespan->race = PlayerRaceState::HUMAN;
		thief->set_creature_level(1);

		// A dwarf thief can find every kind of trap, so it finds them for the cases whose
		// own character could not.
		finder.set_creature_class(CreatureClass::ROGUE);
		finder.set_dexterity(13);
		finder.lifespan->race = PlayerRaceState::DWARF;
		finder.set_creature_level(1);
		finder.thiefSkillPoints.at(thief_skill_index(ThiefSkill::FIND_REMOVE_TRAPS)) = 40;
	}

	// Puts a known, unsprung trap of that kind in front of the thief, found by the
	// finder's search on a roll of 1, which finds a pit on 1d6 and a dart on d100.
	std::unique_ptr<Trap> found_trap(TrapType kind)
	{
		auto trap = std::make_unique<Trap>(Vector2D{ 5, 5 }, kind, mock.tile_config);
		mock.dice.set_next_roll(1);
		EXPECT_EQ(trap->search(finder, ctx), TrapSearchResult::FOUND) << "a search rolling 1 finds the trap";
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

	// A found dart trap on the tile east of the thief, where the disarm action looks.
	Trap& found_trap_to_the_east()
	{
		mock.traps.push_back(found_trap());
		mock.traps.back()->position = EAST_OF_THIEF;
		return *mock.traps.back();
	}

	std::string all_message_text() const
	{
		std::string joined;
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			for (const auto& part : ctx.messageSystem->get_attack_message_at(index))
			{
				joined += part.text;
			}
		}
		return joined;
	}

	// Find/Remove Traps is base 5, so this is what the character rolls against.
	void buy_find_remove_traps(int points)
	{
		thief->thiefSkillPoints.at(thief_skill_index(ThiefSkill::FIND_REMOVE_TRAPS)) = points;
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> thief{ std::make_unique<Player>(Vector2D{ 5, 5 }) };
	Player finder{ Vector2D{ 5, 4 } };
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

// The save-scum the rule was open to. "He can try disarming the trap again when he
// advances to the next experience level" is kept on the trap, and nothing about a trap
// reached the save file, so reloading handed the attempt back. The trap carries the
// level across the save now, and a second try at the same level is still refused.
TEST_F(TrapDisarmTest, AnAttemptIsNotHandedBackByReloading)
{
	buy_find_remove_traps(20);
	auto pit = found_trap();

	mock.dice.set_next_roll(90);
	ASSERT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::BEYOND_SKILL) << "the attempt for this level is spent";

	std::vector<std::unique_ptr<Trap>> level;
	level.push_back(std::move(pit));
	json saved;
	save_traps(level, saved);
	load_traps(saved, level, mock.tile_config);

	ASSERT_EQ(level.size(), 1u);

	// A sure success is queued behind the guard: a trap that remembered the attempt
	// refuses before rolling and never reaches it, while one that forgot rolls the 1
	// and defuses.
	mock.dice.set_next_roll(1);

	EXPECT_EQ(level.front()->attempt_disarm(*thief, ctx), DisarmResult::ALREADY_TRIED)
		<< "reloading gave the thief its attempt back";
}

// A defused trap keeps its sprung face across a save. The tile follows from the state
// rather than being stored, so the load path has to re-derive it - and a trap drawn
// armed is one the player will walk around for no reason.
TEST_F(TrapDisarmTest, ADefusedTrapStillLooksDefusedAfterReloading)
{
	buy_find_remove_traps(20);
	auto pit = found_pit();

	mock.dice.set_next_roll(1);
	ASSERT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::DISARMED);
	const TileRef sprung = pit->actorData.tile;
	ASSERT_FALSE(sprung == mock.tile_config.get("TILE_TRAP_PIT_ARMED")) << "a defused pit does not show the armed face";

	std::vector<std::unique_ptr<Trap>> level;
	level.push_back(std::move(pit));
	json saved;
	save_traps(level, saved);
	load_traps(saved, level, mock.tile_config);

	ASSERT_EQ(level.size(), 1u);
	EXPECT_EQ(level.front()->get_state(), TrapState::DISARMED);
	EXPECT_TRUE(level.front()->actorData.tile == sprung) << "the defused pit came back drawn as armed";
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
	EXPECT_EQ(pit->attempt_disarm(*thief, ctx), DisarmResult::ALREADY_TRIED);

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

// The disarm action works on the trap on the tile it is aimed at, says so, and takes
// the book's 1d10 rounds, rolled after the attempt: "This also requires 1d10 rounds."
TEST_F(TrapDisarmTest, TheDisarmActionWorksOnTheTrapItIsAimedAt)
{
	buy_find_remove_traps(40);
	const Trap& dart = found_trap_to_the_east();
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(7);

	EXPECT_EQ(TrapDisarm::disarm_at(*thief, EAST_OF_THIEF, ctx), std::optional<int>{ 7 });
	EXPECT_EQ(dart.get_state(), TrapState::DISARMED);
	EXPECT_NE(all_message_text().find("You successfully disarm the trap."), std::string::npos) << all_message_text();
}

// A rolled failure and the 96 accident are attempts too, and each takes its 1d10 rounds.
TEST_F(TrapDisarmTest, AFailedOrSprungAttemptStillTakesItsRounds)
{
	buy_find_remove_traps(40);
	const Trap& failed = found_trap_to_the_east();
	mock.dice.set_next_roll(90);
	mock.dice.set_next_roll(4);
	EXPECT_EQ(TrapDisarm::disarm_at(*thief, EAST_OF_THIEF, ctx), std::optional<int>{ 4 });
	EXPECT_EQ(failed.get_state(), TrapState::DETECTED);
	EXPECT_NE(all_message_text().find("This trap is beyond you."), std::string::npos) << all_message_text();

	thief->set_creature_level(2);
	// 96 springs it: a dart for 1, the coin keeps the trap, then the time.
	mock.dice.set_next_roll(96);
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(2);
	mock.dice.set_next_roll(9);
	EXPECT_EQ(TrapDisarm::disarm_at(*thief, EAST_OF_THIEF, ctx), std::optional<int>{ 9 });
	EXPECT_NE(all_message_text().find("You trigger the trap while attempting to disarm it!"), std::string::npos) << all_message_text();
}

// Every refusal is told and costs nothing: no roll is made, neither for the attempt nor
// for the time. A roll is queued ahead of each and must still be the next one out.
TEST_F(TrapDisarmTest, ARefusalRollsNothingAndTakesNoTime)
{
	buy_find_remove_traps(40);

	mock.dice.set_next_roll(7);
	EXPECT_EQ(TrapDisarm::disarm_at(*thief, WEST_OF_THIEF, ctx), std::nullopt);
	EXPECT_EQ(mock.dice.d10(), 7) << "bare floor rolled a die";
	EXPECT_NE(all_message_text().find("There is no trap there."), std::string::npos) << all_message_text();

	auto& hidden = *mock.traps.emplace_back(std::make_unique<Trap>(WEST_OF_THIEF, TrapType::DART, mock.tile_config));
	mock.dice.set_next_roll(7);
	EXPECT_EQ(TrapDisarm::disarm_at(*thief, WEST_OF_THIEF, ctx), std::nullopt);
	EXPECT_EQ(mock.dice.d10(), 7) << "a hidden trap rolled a die";
	EXPECT_EQ(hidden.get_state(), TrapState::HIDDEN);
	EXPECT_NE(all_message_text().find("You don't see a trap there."), std::string::npos) << all_message_text();

	const Trap& tried = found_trap_to_the_east();
	mock.dice.set_next_roll(90);
	mock.dice.set_next_roll(3);
	ASSERT_EQ(TrapDisarm::disarm_at(*thief, EAST_OF_THIEF, ctx), std::optional<int>{ 3 });
	mock.dice.set_next_roll(7);
	EXPECT_EQ(TrapDisarm::disarm_at(*thief, EAST_OF_THIEF, ctx), std::nullopt);
	EXPECT_EQ(mock.dice.d10(), 7) << "a spent attempt rolled a die";
	EXPECT_EQ(tried.get_state(), TrapState::DETECTED);
	EXPECT_NE(all_message_text().find("This trap stays beyond you until you gain a level."), std::string::npos) << all_message_text();

	thief->set_creature_class(CreatureClass::FIGHTER);
	thief->set_creature_level(2);
	mock.dice.set_next_roll(7);
	EXPECT_EQ(TrapDisarm::disarm_at(*thief, EAST_OF_THIEF, ctx), std::nullopt);
	EXPECT_EQ(mock.dice.d10(), 7) << "a fighter rolled a die";
	EXPECT_EQ(tried.get_state(), TrapState::DETECTED);
	EXPECT_NE(all_message_text().find("You cannot work a trap."), std::string::npos) << all_message_text();
}

// A disarmed trap has nothing left to work on.
TEST_F(TrapDisarmTest, ADisarmedTrapIsRefusedWithoutAnAction)
{
	buy_find_remove_traps(40);
	found_trap_to_the_east();
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(2);
	ASSERT_EQ(TrapDisarm::disarm_at(*thief, EAST_OF_THIEF, ctx), std::optional<int>{ 2 });

	mock.dice.set_next_roll(7);
	EXPECT_EQ(TrapDisarm::disarm_at(*thief, EAST_OF_THIEF, ctx), std::nullopt);
	EXPECT_EQ(mock.dice.d10(), 7) << "a disarmed trap rolled a die";
	EXPECT_NE(all_message_text().find("That trap is already disarmed."), std::string::npos) << all_message_text();
}

// end of file: TrapDisarmTest.cpp
