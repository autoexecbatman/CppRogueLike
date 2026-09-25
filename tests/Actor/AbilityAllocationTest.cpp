// file: AbilityAllocationTest.cpp
// Method VI, the Player's Handbook's allocation method (PDF page 24).
//
// The book's whole rule: "Each ability starts with a score of 8. Then roll seven
// dice. These dice can be added to your character's abilities as you wish. All the
// points on a die must be added to the same ability score. For example, if a 6 is
// rolled on one die, all 6 points must be assigned to one ability. You can add as
// many dice as you want to any ability, but no ability score can exceed 18 points.
// If you cannot make an 18 by exact count on the dice, you cannot have an 18 score."
//
// Every expected number below is worked from that paragraph and from Table 13's
// class minimums (PDF page 53), never from what the code returns.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=AbilityAllocationTest.*

#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "src/AbilityAllocation.h"
#include "src/CreatureClass.h"
#include "src/RandomDice.h"

namespace
{
constexpr std::array<int, ABILITY_COUNT> NO_RACE{ 0, 0, 0, 0, 0, 0 };

// A halfling pays a point of Strength for a point of Dexterity.
constexpr std::array<int, ABILITY_COUNT> HALFLING{ -1, 1, 0, 0, 0, 0 };

AbilityAllocation with_pool(std::vector<int> pool, CreatureClass forClass)
{
	return AbilityAllocation{ std::move(pool), forClass, NO_RACE };
}
} // namespace

// "Each ability starts with a score of 8."
TEST(AbilityAllocationTest, EveryAbilityStartsAtEight)
{
	const AbilityAllocation allocation = with_pool({ 4, 4, 2, 1, 6, 3, 5 }, CreatureClass::FIGHTER);

	for (const Ability ability : ALL_ABILITY)
	{
		EXPECT_EQ(allocation.allocated(ability), 8) << ability_name(ability);
	}
	EXPECT_EQ(allocation.pool().size(), 7u);
}

// "All the points on a die must be added to the same ability score": a 6 raises one
// ability by 6, and the die leaves the pool whole.
TEST(AbilityAllocationTest, AWholeDieLandsOnOneAbility)
{
	AbilityAllocation allocation = with_pool({ 6, 3 }, CreatureClass::FIGHTER);

	ASSERT_TRUE(allocation.can_spend(Ability::STRENGTH, 0));
	allocation.spend(Ability::STRENGTH, 0);

	EXPECT_EQ(allocation.allocated(Ability::STRENGTH), 14);
	EXPECT_EQ(allocation.allocated(Ability::DEXTERITY), 8);
	ASSERT_EQ(allocation.pool().size(), 1u);
	EXPECT_EQ(allocation.pool().front(), 3);
}

// "You can add as many dice as you want to any ability."
TEST(AbilityAllocationTest, SeveralDiceMayLandOnOneAbility)
{
	AbilityAllocation allocation = with_pool({ 4, 3, 2 }, CreatureClass::FIGHTER);

	allocation.spend(Ability::STRENGTH, 0);
	allocation.spend(Ability::STRENGTH, 0);
	allocation.spend(Ability::STRENGTH, 0);

	EXPECT_EQ(allocation.allocated(Ability::STRENGTH), 17);
	EXPECT_TRUE(allocation.pool().empty());
}

// "No ability score can exceed 18 points." A 6 onto a 14 would make 20, so that die
// cannot go there at all - the book gives no way to spend part of one.
TEST(AbilityAllocationTest, ADieThatWouldPassEighteenCannotBeSpentThere)
{
	AbilityAllocation allocation = with_pool({ 6, 6, 2 }, CreatureClass::FIGHTER);
	allocation.spend(Ability::STRENGTH, 0);
	ASSERT_EQ(allocation.allocated(Ability::STRENGTH), 14);

	EXPECT_FALSE(allocation.can_spend(Ability::STRENGTH, 0)) << "a 6 would make 20";
	EXPECT_TRUE(allocation.can_spend(Ability::STRENGTH, 1)) << "a 2 makes exactly 16";
	EXPECT_TRUE(allocation.can_spend(Ability::DEXTERITY, 0)) << "the 6 still fits elsewhere";
}

// "If you cannot make an 18 by exact count on the dice, you cannot have an 18 score."
// A pool of 3s reaches 17 and stops: no combination sums to the 10 an 18 needs.
TEST(AbilityAllocationTest, AnEighteenNeedsAnExactCount)
{
	AbilityAllocation allocation = with_pool({ 3, 3, 3, 3 }, CreatureClass::WIZARD);
	allocation.spend(Ability::INTELLIGENCE, 0);
	allocation.spend(Ability::INTELLIGENCE, 0);
	allocation.spend(Ability::INTELLIGENCE, 0);
	ASSERT_EQ(allocation.allocated(Ability::INTELLIGENCE), 17);

	EXPECT_FALSE(allocation.can_spend(Ability::INTELLIGENCE, 0)) << "the fourth 3 would make 20";
	EXPECT_EQ(allocation.allocated(Ability::INTELLIGENCE), 17);
}

// A die exactly filling the gap is allowed, which is what "by exact count" permits.
TEST(AbilityAllocationTest, ADieThatFillsTheGapExactlyMakesEighteen)
{
	AbilityAllocation allocation = with_pool({ 6, 4 }, CreatureClass::FIGHTER);
	allocation.spend(Ability::STRENGTH, 0);

	ASSERT_TRUE(allocation.can_spend(Ability::STRENGTH, 0));
	allocation.spend(Ability::STRENGTH, 0);

	EXPECT_EQ(allocation.allocated(Ability::STRENGTH), 18);
}

// A die taken back comes back as itself, so the pool a player sees never changes
// value - only which side of the screen a die sits on.
TEST(AbilityAllocationTest, ADieTakenBackReturnsToThePoolAsItself)
{
	AbilityAllocation allocation = with_pool({ 5, 2 }, CreatureClass::FIGHTER);
	allocation.spend(Ability::STRENGTH, 0);
	allocation.spend(Ability::STRENGTH, 0);
	ASSERT_EQ(allocation.allocated(Ability::STRENGTH), 15);

	ASSERT_TRUE(allocation.has_placed(Ability::STRENGTH));
	allocation.take_back(Ability::STRENGTH);

	EXPECT_EQ(allocation.allocated(Ability::STRENGTH), 13) << "the last die placed is the one that leaves";
	ASSERT_EQ(allocation.pool().size(), 1u);
	EXPECT_EQ(allocation.pool().front(), 2);
}

TEST(AbilityAllocationTest, AnAbilityWithNoDiceHasNothingToTakeBack)
{
	const AbilityAllocation allocation = with_pool({ 5 }, CreatureClass::FIGHTER);

	EXPECT_FALSE(allocation.has_placed(Ability::STRENGTH));
}

// Table 13: a fighter cannot be played under Strength 9, so an untouched allocation
// is one point short and a single die settles it.
TEST(AbilityAllocationTest, AFighterIsShortUntilItsStrengthMeetsTableThirteen)
{
	AbilityAllocation allocation = with_pool({ 1, 4 }, CreatureClass::FIGHTER);

	const std::optional<ClassMinimum> shortfall = allocation.unmet_minimum();
	ASSERT_TRUE(shortfall.has_value());
	EXPECT_EQ(shortfall->ability, Ability::STRENGTH);
	EXPECT_EQ(shortfall->score, 9);

	allocation.spend(Ability::STRENGTH, 0);

	EXPECT_FALSE(allocation.unmet_minimum().has_value());
}

// Each class reads its own row, and nothing else stops it being accepted.
TEST(AbilityAllocationTest, EachClassIsJudgedOnTheAbilityItsRowNames)
{
	EXPECT_EQ(class_ability_minimum(CreatureClass::FIGHTER)->ability, Ability::STRENGTH);
	EXPECT_EQ(class_ability_minimum(CreatureClass::ROGUE)->ability, Ability::DEXTERITY);
	EXPECT_EQ(class_ability_minimum(CreatureClass::CLERIC)->ability, Ability::WISDOM);
	EXPECT_EQ(class_ability_minimum(CreatureClass::WIZARD)->ability, Ability::INTELLIGENCE);
	EXPECT_EQ(class_ability_minimum(CreatureClass::FIGHTER)->score, 9);
	EXPECT_FALSE(class_ability_minimum(CreatureClass::MONSTER).has_value());
}

// A monster has no row in Table 13, so an allocation for one is never short.
TEST(AbilityAllocationTest, AClassWithNoRowIsNeverShort)
{
	const AbilityAllocation allocation = with_pool({ 1 }, CreatureClass::MONSTER);

	EXPECT_FALSE(allocation.unmet_minimum().has_value());
}

// The race is paid after the character is built, so a halfling fighter reads one
// Strength lower than it allocated and Table 13 judges the lower number. Allocating
// to 9 leaves it short; 10 is what a halfling fighter has to reach.
TEST(AbilityAllocationTest, ARaceIsShownAndJudgedButNotAllocated)
{
	AbilityAllocation halflingFighter{ { 1, 1 }, CreatureClass::FIGHTER, HALFLING };
	halflingFighter.spend(Ability::STRENGTH, 0);

	EXPECT_EQ(halflingFighter.allocated(Ability::STRENGTH), 9) << "the race is not spent from the pool";
	EXPECT_EQ(halflingFighter.score(Ability::STRENGTH), 8) << "the player reads what it will have";
	EXPECT_EQ(halflingFighter.score(Ability::DEXTERITY), 9) << "and what the race gives it";
	EXPECT_TRUE(halflingFighter.unmet_minimum().has_value()) << "9 allocated is 8 played";

	halflingFighter.spend(Ability::STRENGTH, 0);

	EXPECT_EQ(halflingFighter.score(Ability::STRENGTH), 9);
	EXPECT_FALSE(halflingFighter.unmet_minimum().has_value());
}

// The blueprint is built from what was allocated, not from what the player read,
// because racial_ability_adjustments pays the race once the character exists.
TEST(AbilityAllocationTest, TheScoresHandedOnAreTheAllocatedOnes)
{
	AbilityAllocation halflingFighter{ { 4 }, CreatureClass::FIGHTER, HALFLING };
	halflingFighter.spend(Ability::STRENGTH, 0);

	const std::array<int, ABILITY_COUNT> scores = halflingFighter.allocated_scores();

	EXPECT_EQ(scores.at(ability_index(Ability::STRENGTH)), 12);
	EXPECT_EQ(scores.at(ability_index(Ability::DEXTERITY)), 8);
	EXPECT_EQ(scores.at(ability_index(Ability::CHARISMA)), 8);
}

// Seven six-sided dice, kept in the order they came up so a screen can show them.
TEST(AbilityAllocationTest, ThePoolIsSevenSixSidedDice)
{
	RandomDice dice{};
#ifdef TESTING_MODE
	dice.set_test_mode(true);
	for (const int forced : { 4, 4, 2, 1, 6, 3, 5 })
	{
		dice.set_next_roll(forced);
	}
#endif

	const std::vector<int> pool = roll_method_six_pool(dice);

	ASSERT_EQ(pool.size(), static_cast<std::size_t>(METHOD_SIX_DICE));
#ifdef TESTING_MODE
	EXPECT_EQ(pool, (std::vector<int>{ 4, 4, 2, 1, 6, 3, 5 }));
#endif
}

// The cursor the screen walks the six with.
TEST(AbilityAllocationTest, TheAbilityListIsWalkedInBothDirectionsAndWraps)
{
	EXPECT_EQ(next_ability(Ability::STRENGTH), Ability::DEXTERITY);
	EXPECT_EQ(next_ability(Ability::CHARISMA), Ability::STRENGTH);
	EXPECT_EQ(previous_ability(Ability::DEXTERITY), Ability::STRENGTH);
	EXPECT_EQ(previous_ability(Ability::STRENGTH), Ability::CHARISMA);
	EXPECT_EQ(ability_name(Ability::CONSTITUTION), "Constitution");
	EXPECT_EQ(ability_index(Ability::CHARISMA), 5u);
}

// 18 is the ceiling, not 19: a score sitting at exactly 18 takes no further die,
// however small.
TEST(AbilityAllocationTest, NothingGoesOnAnAbilityAlreadyAtEighteen)
{
	AbilityAllocation allocation = with_pool({ 6, 4, 1 }, CreatureClass::FIGHTER);
	allocation.spend(Ability::STRENGTH, 0);
	allocation.spend(Ability::STRENGTH, 0);
	ASSERT_EQ(allocation.allocated(Ability::STRENGTH), ABILITY_MAXIMUM);

	EXPECT_FALSE(allocation.can_spend(Ability::STRENGTH, 0)) << "even a 1 would make 19";
	EXPECT_TRUE(allocation.can_spend(Ability::DEXTERITY, 0));
}

// The screen offers seven digits while the pool shrinks under them, so a digit past
// the end of the pool is an ordinary keystroke rather than an impossible one.
TEST(AbilityAllocationTest, ADigitPastTheEndOfThePoolSpendsNothing)
{
	const AbilityAllocation allocation = with_pool({ 5, 2 }, CreatureClass::FIGHTER);

	EXPECT_FALSE(allocation.can_spend(Ability::STRENGTH, 2));
	EXPECT_FALSE(allocation.can_spend(Ability::STRENGTH, 6));
}
