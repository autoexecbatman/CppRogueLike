// file: AbilityAllocationTextTest.cpp
// The lines the ability allocation screen prints, and the one piece of its key
// handling that is arithmetic rather than drawing.
//
// The rules are AbilityAllocationTest's. What is checked here is what the screen
// makes of them: the row format, the race shown only where it does something, the
// pool behind the digits that spend it, the status line, and which die a typed
// character names. Until these were free functions the screen was judged by eye
// through docs/shoot_game.py, which found a box three rows too tall and a hint line
// cut off at "[Enter]" - real defects, found twice, by looking.
//
// Every expected string here is worked out from the std::format specification in
// MenuAbilityScores.cpp rather than copied from a run.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=AbilityAllocationTextTest.*

#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "src/AbilityAllocation.h"
#include "src/CreatureClass.h"
#include "src/MenuAbilityScores.h"

namespace
{
// The seven dice Method VI rolled, in the order they came up. Fixed rather than
// rolled, so every line below is a statement about the format and not about luck.
const std::vector<int> ROLLED_POOL{ 4, 4, 2, 1, 6, 3, 5 };

// Table 2's halfling: -1 Strength, +1 Dexterity, and nothing else. The only race
// here that touches Strength, which is what makes it the useful one to print.
constexpr std::array<int, ABILITY_COUNT> HALFLING{ -1, 1, 0, 0, 0, 0 };

// A human, so that the quiet form of the row has something to be measured on.
constexpr std::array<int, ABILITY_COUNT> HUMAN{ 0, 0, 0, 0, 0, 0 };
} // namespace

// An ability no race touches prints its name and its allocation and stops: there is
// nothing for the modifier half of the line to say.
TEST(AbilityAllocationTextTest, AnUnmodifiedAbilityPrintsNoRacialHalf)
{
	const AbilityAllocation allocation{ ROLLED_POOL, CreatureClass::FIGHTER, HUMAN };

	EXPECT_EQ(
		ability_row_line(allocation, Ability::CONSTITUTION, Ability::STRENGTH),
		"  Constitution   8");
}

// The cursor is the first character of the line, so the marker moves with it and
// the columns do not.
TEST(AbilityAllocationTextTest, TheCursorMarkerIsTheFirstCharacterAndShiftsNothing)
{
	const AbilityAllocation allocation{ ROLLED_POOL, CreatureClass::FIGHTER, HUMAN };

	const std::string away = ability_row_line(allocation, Ability::CONSTITUTION, Ability::STRENGTH);
	const std::string on = ability_row_line(allocation, Ability::CONSTITUTION, Ability::CONSTITUTION);

	EXPECT_EQ(on, "> Constitution   8");
	EXPECT_EQ(away.size(), on.size()) << "the marker changed the width of the row";
	EXPECT_EQ(away.substr(1), on.substr(1)) << "only the first character may differ";
}

// Where the race does something the line shows all three numbers: what the dice
// put there, what the race does to it, and what the character will be built with.
TEST(AbilityAllocationTextTest, AModifiedAbilityShowsTheDiceTheRaceAndTheResult)
{
	AbilityAllocation allocation{ ROLLED_POOL, CreatureClass::FIGHTER, HALFLING };

	// The 6 is the fifth die, so the starting 8 becomes 14 and the halfling -1
	// leaves 13.
	ASSERT_TRUE(allocation.can_spend(Ability::STRENGTH, 4));
	allocation.spend(Ability::STRENGTH, 4);

	EXPECT_EQ(
		ability_row_line(allocation, Ability::STRENGTH, Ability::CONSTITUTION),
		"  Strength      14 -1 = 13");
}

// A positive modifier carries its sign, so the two halves of Table 2 read alike.
TEST(AbilityAllocationTextTest, APositiveModifierCarriesItsSign)
{
	AbilityAllocation allocation{ ROLLED_POOL, CreatureClass::FIGHTER, HALFLING };

	// The 2 is the third die: 8 becomes 10, and the halfling +1 leaves 11.
	ASSERT_TRUE(allocation.can_spend(Ability::DEXTERITY, 2));
	allocation.spend(Ability::DEXTERITY, 2);

	EXPECT_EQ(
		ability_row_line(allocation, Ability::DEXTERITY, Ability::DEXTERITY),
		"> Dexterity     10 +1 = 11");
}

// Each die sits behind the digit that spends it, numbered from one, and the
// numbering closes up when a die leaves the pool.
TEST(AbilityAllocationTextTest, ThePoolNumbersTheDiceFromOneAndClosesUp)
{
	AbilityAllocation allocation{ ROLLED_POOL, CreatureClass::FIGHTER, HUMAN };

	EXPECT_EQ(dice_pool_line(allocation), "Dice: 1)4 2)4 3)2 4)1 5)6 6)3 7)5");

	allocation.spend(Ability::STRENGTH, 4);

	EXPECT_EQ(dice_pool_line(allocation), "Dice: 1)4 2)4 3)2 4)1 5)3 6)5");
}

// An empty pool says so in words. An empty label after "Dice:" would read as a
// drawing fault rather than as a finished allocation.
TEST(AbilityAllocationTextTest, AnEmptyPoolSaysSoRatherThanPrintingNothing)
{
	AbilityAllocation allocation{ ROLLED_POOL, CreatureClass::FIGHTER, HUMAN };

	// One die to each ability in turn, then the last onto Strength, which has room
	// for it: 4 and 5 leave it at 17, under the 18 ceiling.
	allocation.spend(Ability::STRENGTH, 0);
	allocation.spend(Ability::DEXTERITY, 0);
	allocation.spend(Ability::CONSTITUTION, 0);
	allocation.spend(Ability::INTELLIGENCE, 0);
	allocation.spend(Ability::WISDOM, 0);
	allocation.spend(Ability::CHARISMA, 0);
	ASSERT_TRUE(allocation.can_spend(Ability::STRENGTH, 0));
	allocation.spend(Ability::STRENGTH, 0);

	ASSERT_TRUE(allocation.pool().empty());
	EXPECT_EQ(dice_pool_line(allocation), "Dice: none left");
}

// The status line names Table 13's row, and names it on the score the race will
// leave rather than on the dice: a halfling fighter at the starting 8 reads 7.
TEST(AbilityAllocationTextTest, TheStatusLineNamesTheUnmetMinimumThenReportsReady)
{
	AbilityAllocation allocation{ ROLLED_POOL, CreatureClass::FIGHTER, HALFLING };

	EXPECT_EQ(allocation_status_line(allocation), "Needs Strength 9");

	allocation.spend(Ability::STRENGTH, 4);

	EXPECT_EQ(allocation_status_line(allocation), "Ready.");
}

// '1' is the first die and the method rolls seven, so '8' names none. The input
// system reports zero for no key at all, which must not come out as a die either.
TEST(AbilityAllocationTextTest, ADigitNamesADieOnlyInsideThePoolsRange)
{
	EXPECT_EQ(die_index_for_character('1'), 0u);
	EXPECT_EQ(die_index_for_character('7'), 6u);

	EXPECT_FALSE(die_index_for_character('8').has_value());
	EXPECT_FALSE(die_index_for_character('0').has_value());
	EXPECT_FALSE(die_index_for_character(0).has_value());
	EXPECT_FALSE(die_index_for_character('a').has_value());
}

// The last digit the hint line offers is the last die the method rolls, so the two
// cannot drift apart.
TEST(AbilityAllocationTextTest, TheLastSpendableDigitIsTheLastDieRolled)
{
	const int lastDigit = '1' + METHOD_SIX_DICE - 1;

	EXPECT_EQ(die_index_for_character(lastDigit), static_cast<std::size_t>(METHOD_SIX_DICE - 1));
	EXPECT_FALSE(die_index_for_character(lastDigit + 1).has_value());
}
