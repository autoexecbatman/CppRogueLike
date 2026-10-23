// file: LifespanTest.cpp
// A character's age and what it does to ability scores, by the Player's Handbook.
//
// - Table 11 (PDF page 51): starting age as base plus a die roll, and the maximum age.
// - Table 12 (PDF page 51): the middle, old and venerable thresholds, typed here from the
//   printed table, and the changes each brings: "-1 Str/Con; +1 Int/Wis", "-2 Str/Dex,
//   -1 Con; +1 Wis", "-1 Str/Dex/Con; +1 Int/Wis". "All aging adjustments are cumulative."
// - PDF page 49, exceptional Strength: middle age costs "half of his exceptional Strength
//   rating" instead of the point, old age "all his exceptional Strength and 1 point more".
// - PDF page 50, magical ageing: the physical losses, without the Intelligence and Wisdom
//   gains, which "are a function of the passage of game time".
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=LifespanTest.*

#include <gtest/gtest.h>

#include <array>
#include <string_view>

#include "src/AbilityAllocation.h"
#include "src/Lifespan.h"
#include "src/PlayerRaceState.h"
#include "src/RandomDice.h"
#include "tests/mocks/TestDice.h"

namespace
{
// A human of the given ages, living to 100 so nothing here dies.
Lifespan human(int naturalAge, int magicalYears)
{
	return Lifespan{ PlayerRaceState::HUMAN, naturalAge, 100, 0, magicalYears };
}

// Scripts every die of a roll to the same face.
void script_dice(RandomDice& dice, int count, int face)
{
	for (int die = 0; die < count; ++die)
	{
		dice.set_next_roll(face);
	}
}

struct PrintedRow
{
	PlayerRaceState race{ PlayerRaceState::NONE };
	int middleAge{ 0 };
	int oldAge{ 0 };
	int venerableAge{ 0 };
};

// Table 12 as printed.
constexpr std::array<PrintedRow, 6> TABLE_12 = { {
	{ PlayerRaceState::DWARF, 125, 167, 250 },
	{ PlayerRaceState::ELF, 175, 233, 350 },
	{ PlayerRaceState::GNOME, 100, 133, 200 },
	{ PlayerRaceState::HALFELF, 62, 83, 125 },
	{ PlayerRaceState::HALFLING, 50, 67, 100 },
	{ PlayerRaceState::HUMAN, 45, 60, 90 },
} };
} // namespace

TEST(LifespanTest, NaturalAgeIsTheStartPlusTheYearsLived)
{
	const Lifespan lifespan{ PlayerRaceState::HUMAN, 18, 100, 2, 5 };

	EXPECT_EQ(lifespan.natural_age(), 20);
	EXPECT_EQ(lifespan.physical_age(), 25);
}

TEST(LifespanTest, TheThresholdsAreTable12AsPrinted)
{
	for (const PrintedRow& printed : TABLE_12)
	{
		const auto row = lifespan_table(printed.race);
		ASSERT_TRUE(row.has_value())
			<< "no row for race " << static_cast<int>(printed.race);
		EXPECT_EQ(row->middleAge, printed.middleAge)
			<< "race " << static_cast<int>(printed.race);
		EXPECT_EQ(row->oldAge, printed.oldAge)
			<< "race " << static_cast<int>(printed.race);
		EXPECT_EQ(row->venerableAge, printed.venerableAge)
			<< "race " << static_cast<int>(printed.race);
	}
}

TEST(LifespanTest, ARaceWithNoRowNeverAges)
{
	const Lifespan unraced{ PlayerRaceState::NONE, 0, 0, 0, 500 };

	EXPECT_FALSE(lifespan_table(PlayerRaceState::NONE).has_value());
	EXPECT_EQ(age_category(500, PlayerRaceState::NONE), AgeCategory::YOUNG);
	EXPECT_EQ(ageing_change(unraced, Ability::STRENGTH, 0), 0);
}

TEST(LifespanTest, AHumansBandsFollowTheTable)
{
	EXPECT_EQ(age_category(44, PlayerRaceState::HUMAN), AgeCategory::YOUNG);
	EXPECT_EQ(
		age_category(45, PlayerRaceState::HUMAN),
		AgeCategory::MIDDLE_AGE);
	EXPECT_EQ(age_category(60, PlayerRaceState::HUMAN), AgeCategory::OLD_AGE);
	EXPECT_EQ(age_category(90, PlayerRaceState::HUMAN), AgeCategory::VENERABLE);
	EXPECT_EQ(
		age_category(62, PlayerRaceState::HALFELF),
		AgeCategory::MIDDLE_AGE);
	EXPECT_EQ(age_category(61, PlayerRaceState::HALFELF), AgeCategory::YOUNG);
	EXPECT_EQ(age_category_name(AgeCategory::OLD_AGE), "old age");
}

TEST(LifespanTest, AHumanOf44HasNoChange)
{
	for (const Ability ability : ALL_ABILITY)
	{
		EXPECT_EQ(ageing_change(human(44, 0), ability, 0), 0)
			<< "ability " << ability_index(ability);
	}
}

TEST(LifespanTest, MiddleAgeByTimeChangesBodyAndMind)
{
	const Lifespan aged = human(45, 0);

	EXPECT_EQ(ageing_change(aged, Ability::STRENGTH, 0), -1);
	EXPECT_EQ(ageing_change(aged, Ability::DEXTERITY, 0), 0);
	EXPECT_EQ(ageing_change(aged, Ability::CONSTITUTION, 0), -1);
	EXPECT_EQ(ageing_change(aged, Ability::INTELLIGENCE, 0), 1);
	EXPECT_EQ(ageing_change(aged, Ability::WISDOM, 0), 1);
	EXPECT_EQ(ageing_change(aged, Ability::CHARISMA, 0), 0);
}

// Fifteen magical years on a body of 30: the body is 45 and the mind is 30.
TEST(LifespanTest, MiddleAgeByMagicChangesOnlyTheBody)
{
	const Lifespan aged = human(30, 15);

	EXPECT_EQ(ageing_change(aged, Ability::STRENGTH, 0), -1);
	EXPECT_EQ(ageing_change(aged, Ability::CONSTITUTION, 0), -1);
	EXPECT_EQ(ageing_change(aged, Ability::INTELLIGENCE, 0), 0);
	EXPECT_EQ(ageing_change(aged, Ability::WISDOM, 0), 0);
}

TEST(LifespanTest, TheBandsAccumulate)
{
	const Lifespan aged = human(60, 0);

	EXPECT_EQ(ageing_change(aged, Ability::STRENGTH, 0), -3);
	EXPECT_EQ(ageing_change(aged, Ability::DEXTERITY, 0), -2);
	EXPECT_EQ(ageing_change(aged, Ability::CONSTITUTION, 0), -2);
	EXPECT_EQ(ageing_change(aged, Ability::INTELLIGENCE, 0), 1);
	EXPECT_EQ(ageing_change(aged, Ability::WISDOM, 0), 2);

	const Lifespan venerable = human(90, 0);
	EXPECT_EQ(ageing_change(venerable, Ability::STRENGTH, 0), -4);
	EXPECT_EQ(ageing_change(venerable, Ability::DEXTERITY, 0), -3);
	EXPECT_EQ(ageing_change(venerable, Ability::CONSTITUTION, 0), -3);
	EXPECT_EQ(ageing_change(venerable, Ability::INTELLIGENCE, 0), 2);
	EXPECT_EQ(ageing_change(venerable, Ability::WISDOM, 0), 3);
}

// An 18/50 at middle age keeps Strength 18 at 18/25; at old age it is 17 with nothing left.
TEST(LifespanTest, ExceptionalStrengthIsSpentBeforeThePoints)
{
	EXPECT_EQ(ageing_change(human(45, 0), Ability::STRENGTH, 50), 0);
	EXPECT_EQ(aged_exceptional_strength(human(45, 0), 50), 25);

	EXPECT_EQ(ageing_change(human(60, 0), Ability::STRENGTH, 50), -1);
	EXPECT_EQ(aged_exceptional_strength(human(60, 0), 50), 0);

	EXPECT_EQ(aged_exceptional_strength(human(44, 0), 50), 50);
}

// Magic taking years back restores the body; the mind keeps what time gave it.
TEST(LifespanTest, YearsTakenBackRestoreTheBodyAndKeepTheMind)
{
	const Lifespan restored = human(50, -10);

	EXPECT_EQ(ageing_change(restored, Ability::STRENGTH, 0), 0);
	EXPECT_EQ(ageing_change(restored, Ability::CONSTITUTION, 0), 0);
	EXPECT_EQ(ageing_change(restored, Ability::INTELLIGENCE, 0), 1);
	EXPECT_EQ(ageing_change(restored, Ability::WISDOM, 0), 1);
}

TEST(LifespanTest, AHumanStartsAt16To19AndLives92To130)
{
	RandomDice least{ TEST_DICE_SEED };
	script_dice(least, 3, 1);
	EXPECT_EQ(roll_lifespan(PlayerRaceState::HUMAN, least).startingAge, 16);
	RandomDice leastAgain{ TEST_DICE_SEED };
	script_dice(leastAgain, 3, 1);
	EXPECT_EQ(roll_lifespan(PlayerRaceState::HUMAN, leastAgain).maximumAge, 92);

	RandomDice most{ TEST_DICE_SEED };
	most.set_next_roll(4);
	script_dice(most, 2, 20);
	const Lifespan oldest = roll_lifespan(PlayerRaceState::HUMAN, most);
	EXPECT_EQ(oldest.startingAge, 19);
	EXPECT_EQ(oldest.maximumAge, 130);
	EXPECT_EQ(oldest.yearsLived, 0);
	EXPECT_EQ(oldest.magicalYears, 0);
	EXPECT_EQ(oldest.race, PlayerRaceState::HUMAN);
}

TEST(LifespanTest, ElvesAndDwarvesStartAtTheirOwnAges)
{
	RandomDice elfLeast{ TEST_DICE_SEED };
	script_dice(elfLeast, 9, 1);
	EXPECT_EQ(roll_lifespan(PlayerRaceState::ELF, elfLeast).startingAge, 105);

	RandomDice elfMost{ TEST_DICE_SEED };
	script_dice(elfMost, 5, 6);
	script_dice(elfMost, 4, 1);
	EXPECT_EQ(roll_lifespan(PlayerRaceState::ELF, elfMost).startingAge, 130);

	RandomDice dwarfLeast{ TEST_DICE_SEED };
	script_dice(dwarfLeast, 7, 1);
	EXPECT_EQ(
		roll_lifespan(PlayerRaceState::DWARF, dwarfLeast).startingAge,
		45);

	RandomDice dwarfMost{ TEST_DICE_SEED };
	script_dice(dwarfMost, 5, 6);
	script_dice(dwarfMost, 2, 1);
	EXPECT_EQ(roll_lifespan(PlayerRaceState::DWARF, dwarfMost).startingAge, 70);
}

// end of file: LifespanTest.cpp
