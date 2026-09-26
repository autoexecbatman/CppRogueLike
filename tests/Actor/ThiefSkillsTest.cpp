// file: ThiefSkillsTest.cpp
// The thief's eight skills, from Player's Handbook Tables 26 to 29 (PDF pages 84
// to 86). Every expected number below is read off those tables, never off the code.
//
// Table 26, base scores: Pick Pockets 15, Open Locks 10, Find/Remove Traps 5,
// Move Silently 10, Hide in Shadows 5, Detect Noise 15, Climb Walls 60,
// Read Languages 0.
//
// The discretionary points are the book's own: "all thieves at 1st level receive 60
// discretionary percentage points that they can add to their base scores. No more
// than 30 points can be assigned to any single skill... Each time the thief rises a
// level in experience, the player receives another 30 points to distribute. No more
// than 15 points per level can be assigned to a single skill, and no skill can be
// raised above 95 percent, including all adjustments for Dexterity, race, and armor."
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ThiefSkillsTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "src/Paths.h"
#include "src/ThiefSkills.h"

namespace
{
constexpr std::array<int, THIEF_SKILL_COUNT> NO_RACE{ 0, 0, 0, 0, 0, 0, 0, 0 };

// Table 27's dwarf column, in ALL_THIEF_SKILL order.
constexpr std::array<int, THIEF_SKILL_COUNT> DWARF{ 0, 10, 15, 0, 0, 0, -10, -5 };

// Table 27's halfling column.
constexpr std::array<int, THIEF_SKILL_COUNT> HALFLING{ 5, 5, 5, 10, 15, 5, -15, -5 };

ThiefSkillAllocation first_level(
	std::array<int, THIEF_SKILL_COUNT> race,
	int dexterity,
	ThiefArmor armor)
{
	constexpr std::array<int, THIEF_SKILL_COUNT> NOTHING_SPENT{ 0, 0, 0, 0, 0, 0, 0, 0 };
	return ThiefSkillAllocation{ NOTHING_SPENT, thief_skill_grant_at_level(1), race, dexterity, armor };
}
} // namespace

// Table 26, read straight down.
TEST(ThiefSkillsTest, TableTwentySixGivesEachSkillItsBaseScore)
{
	EXPECT_EQ(thief_skill_base(ThiefSkill::PICK_POCKETS), 15);
	EXPECT_EQ(thief_skill_base(ThiefSkill::OPEN_LOCKS), 10);
	EXPECT_EQ(thief_skill_base(ThiefSkill::FIND_REMOVE_TRAPS), 5);
	EXPECT_EQ(thief_skill_base(ThiefSkill::MOVE_SILENTLY), 10);
	EXPECT_EQ(thief_skill_base(ThiefSkill::HIDE_IN_SHADOWS), 5);
	EXPECT_EQ(thief_skill_base(ThiefSkill::DETECT_NOISE), 15);
	EXPECT_EQ(thief_skill_base(ThiefSkill::CLIMB_WALLS), 60);
	EXPECT_EQ(thief_skill_base(ThiefSkill::READ_LANGUAGES), 0);
}

// Table 28's five columns. The three skills with no column are unaffected, which
// is a claim about the table rather than an absence: Detect Noise, Climb Walls and
// Read Languages are printed nowhere in it.
TEST(ThiefSkillsTest, TableTwentyEightAdjustsFiveSkillsAndLeavesThreeAlone)
{
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::PICK_POCKETS, 9), -15);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 9), -10);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::FIND_REMOVE_TRAPS, 9), -10);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::MOVE_SILENTLY, 9), -20);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::HIDE_IN_SHADOWS, 9), -10);

	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::PICK_POCKETS, 12), 0);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 12), 0);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::FIND_REMOVE_TRAPS, 12), 0);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::MOVE_SILENTLY, 12), -5);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::HIDE_IN_SHADOWS, 12), 0);

	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::PICK_POCKETS, 19), 15);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 19), 20);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::FIND_REMOVE_TRAPS, 19), 10);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::MOVE_SILENTLY, 19), 15);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::HIDE_IN_SHADOWS, 19), 15);

	for (int dexterity = 9; dexterity <= 19; ++dexterity)
	{
		EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::DETECT_NOISE, dexterity), 0) << dexterity;
		EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::CLIMB_WALLS, dexterity), 0) << dexterity;
		EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::READ_LANGUAGES, dexterity), 0) << dexterity;
	}
}

// The rows the old hand-written ladder got wrong. It read Dexterity 10 as -10,
// 11 and 12 as -5, 15 as +5 and everything from 17 up as +10; Table 28's Open
// Locks column says -5, 0, 0, 0, and 10, 15, 20 at 17, 18 and 19.
TEST(ThiefSkillsTest, TableTwentyEightOpenLocksRunsRowByRow)
{
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 10), -5);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 11), 0);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 13), 0);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 15), 0);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 16), 5);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 17), 10);
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 18), 15);
}

// The book prints 9 through 19. Gauntlets of dexterity reach past it, and the
// edge row holds rather than the lookup falling off the end.
TEST(ThiefSkillsTest, ADexterityOffTheTableReadsTheNearestPrintedRow)
{
	// Row 19's Open Locks is +20, and 25 is above it.
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::OPEN_LOCKS, 25), 20);

	// Row 9's Move Silently is -20, and 1 is below it.
	EXPECT_EQ(thief_skill_dexterity_adjustment(ThiefSkill::MOVE_SILENTLY, 1), -20);
}

// Table 29. Leather is the base scores' own assumption, so it adjusts nothing;
// wearing nothing is a bonus column.
TEST(ThiefSkillsTest, TableTwentyNineAdjustsForWhatIsWorn)
{
	for (const ThiefSkill skill : ALL_THIEF_SKILL)
	{
		EXPECT_EQ(thief_skill_armor_adjustment(skill, ThiefArmor::LEATHER), 0) << thief_skill_name(skill);
	}

	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::PICK_POCKETS, ThiefArmor::NONE), 5);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::MOVE_SILENTLY, ThiefArmor::NONE), 10);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::HIDE_IN_SHADOWS, ThiefArmor::NONE), 5);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::CLIMB_WALLS, ThiefArmor::NONE), 10);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::OPEN_LOCKS, ThiefArmor::NONE), 0);

	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::PICK_POCKETS, ThiefArmor::PADDED_OR_ELVEN_CHAIN), -20);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::OPEN_LOCKS, ThiefArmor::PADDED_OR_ELVEN_CHAIN), -5);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::CLIMB_WALLS, ThiefArmor::PADDED_OR_ELVEN_CHAIN), -20);

	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::PICK_POCKETS, ThiefArmor::HIDE_OR_STUDDED_LEATHER), -30);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::DETECT_NOISE, ThiefArmor::HIDE_OR_STUDDED_LEATHER), -10);

	// Chain is kinder than studded leather on five of the eight, which is the
	// table's own shape rather than a transcription slip.
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::PICK_POCKETS, ThiefArmor::CHAIN_OR_RING_MAIL), -25);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::MOVE_SILENTLY, ThiefArmor::CHAIN_OR_RING_MAIL), -15);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::DETECT_NOISE, ThiefArmor::CHAIN_OR_RING_MAIL), -5);

	// Read Languages is the one row the whole table leaves alone.
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::READ_LANGUAGES, ThiefArmor::NONE), 0);
	EXPECT_EQ(thief_skill_armor_adjustment(ThiefSkill::READ_LANGUAGES, ThiefArmor::CHAIN_OR_RING_MAIL), 0);
}

// The armour the game ships, read into Table 29's columns by its item key.
TEST(ThiefSkillsTest, EachArmorTheDataShipsFindsItsColumn)
{
	EXPECT_EQ(thief_armor_column("leather_armor"), ThiefArmor::LEATHER);
	EXPECT_EQ(thief_armor_column("padded_armor"), ThiefArmor::PADDED_OR_ELVEN_CHAIN);
	EXPECT_EQ(thief_armor_column("studded_leather"), ThiefArmor::HIDE_OR_STUDDED_LEATHER);
	EXPECT_EQ(thief_armor_column("hide_armor"), ThiefArmor::HIDE_OR_STUDDED_LEATHER);
	EXPECT_EQ(thief_armor_column("chain_mail"), ThiefArmor::CHAIN_OR_RING_MAIL);
	EXPECT_EQ(thief_armor_column("ring_mail"), ThiefArmor::CHAIN_OR_RING_MAIL);

	// Table 29 prints no column for anything heavier, and the book does not let a
	// thief wear it either.
	EXPECT_FALSE(thief_armor_column("plate_mail").has_value());
	EXPECT_FALSE(thief_armor_column("full_plate").has_value());
	EXPECT_FALSE(thief_armor_column("splint_mail").has_value());
	EXPECT_FALSE(thief_armor_column("no_such_item").has_value());
}

// The keys above are the data's, so a rename must not leave the table pointing at
// nothing. This walks items.json rather than trusting the spelling: every armour
// the book gives a column has to still be there under that key, and every other
// suit of armour the game ships has to come back with no column.
TEST(ThiefSkillsTest, EveryArmorTheDataShipsIsAnsweredForByTableTwentyNine)
{
	std::ifstream file(Paths::resolve(Paths::ITEMS));
	ASSERT_TRUE(file.is_open()) << "items.json did not open";

	nlohmann::json items;
	file >> items;

	constexpr std::array<std::string_view, 6> WITH_A_COLUMN{
		"leather_armor", "padded_armor", "studded_leather", "hide_armor", "chain_mail", "ring_mail"
	};

	int found = 0;
	for (const auto& [key, record] : items.items())
	{
		if (record.at("category").get<std::string>() != "armor")
		{
			continue;
		}

		const bool bookGivesAColumn = std::ranges::find(WITH_A_COLUMN, key) != WITH_A_COLUMN.end();
		if (bookGivesAColumn)
		{
			++found;
			EXPECT_TRUE(thief_armor_column(key).has_value()) << key << " lost its Table 29 column";
		}
		else
		{
			EXPECT_FALSE(thief_armor_column(key).has_value())
				<< key << " was given a column Table 29 does not print";
		}
	}

	EXPECT_EQ(found, static_cast<int>(WITH_A_COLUMN.size()))
		<< "an armour thief_armor_column names is no longer in items.json under that key";
}

// Base, race, Dexterity, armour and points, added the way the book adds them.
TEST(ThiefSkillsTest, AScoreIsTheBaseAndEveryAdjustmentAndThePointsSpent)
{
	// A halfling thief, Dexterity 17, in leather, 20 points on Hide in Shadows:
	// base 5, halfling +15, Dexterity +5, leather 0, points 20.
	EXPECT_EQ(thief_skill_score(ThiefSkill::HIDE_IN_SHADOWS, 15, 17, ThiefArmor::LEATHER, 20), 45);

	// The same halfling wearing nothing gains Table 29's +5.
	EXPECT_EQ(thief_skill_score(ThiefSkill::HIDE_IN_SHADOWS, 15, 17, ThiefArmor::NONE, 20), 50);

	// A dwarf opening a lock: base 10, dwarf +10, Dexterity 17 +10, leather 0.
	EXPECT_EQ(thief_skill_score(ThiefSkill::OPEN_LOCKS, 10, 17, ThiefArmor::LEATHER, 0), 30);
}

// "no skill can be raised above 95 percent, including all adjustments".
TEST(ThiefSkillsTest, NoScoreRisesAboveNinetyFive)
{
	EXPECT_EQ(thief_skill_score(ThiefSkill::CLIMB_WALLS, 0, 13, ThiefArmor::NONE, 60), 95);
}

// "the character must spend points raising his skill percentage to at least 1%
// before he can use the skill" - so a negative total reads as a skill not yet had.
TEST(ThiefSkillsTest, AdjustmentsBelowZeroLeaveNoSkillAtAll)
{
	// An elf, Dexterity 9, in chain: base 10, elf -5, Dexterity -10, chain -10.
	EXPECT_EQ(thief_skill_score(ThiefSkill::OPEN_LOCKS, -5, 9, ThiefArmor::CHAIN_OR_RING_MAIL, 0), 0);

	// Twenty points buys it back up above nothing.
	EXPECT_EQ(thief_skill_score(ThiefSkill::OPEN_LOCKS, -5, 9, ThiefArmor::CHAIN_OR_RING_MAIL, 20), 5);
}

// "all thieves at 1st level receive 60 discretionary percentage points... Each
// time the thief rises a level in experience, the player receives another 30."
TEST(ThiefSkillsTest, TheGrantIsSixtyAtFirstLevelAndThirtyAfterwards)
{
	EXPECT_EQ(thief_skill_grant_at_level(1).points, 60);
	EXPECT_EQ(thief_skill_grant_at_level(1).perSkillLimit, 30);

	for (int level = 2; level <= 10; ++level)
	{
		EXPECT_EQ(thief_skill_grant_at_level(level).points, 30) << level;
		EXPECT_EQ(thief_skill_grant_at_level(level).perSkillLimit, 15) << level;
	}
}

TEST(ThiefSkillsTest, AFreshAllocationHoldsTheWholeGrantAndNothingIsAssigned)
{
	const ThiefSkillAllocation allocation = first_level(NO_RACE, 13, ThiefArmor::NONE);

	EXPECT_EQ(allocation.remaining_points(), 60);
	for (const ThiefSkill skill : ALL_THIEF_SKILL)
	{
		EXPECT_EQ(allocation.assigned_to(skill), 0) << thief_skill_name(skill);
	}
}

TEST(ThiefSkillsTest, AssigningAPointTakesItFromThePoolAndRaisesTheScore)
{
	ThiefSkillAllocation allocation = first_level(NO_RACE, 13, ThiefArmor::LEATHER);

	const int before = allocation.score(ThiefSkill::OPEN_LOCKS);
	ASSERT_TRUE(allocation.can_assign(ThiefSkill::OPEN_LOCKS));
	allocation.assign(ThiefSkill::OPEN_LOCKS);

	EXPECT_EQ(allocation.assigned_to(ThiefSkill::OPEN_LOCKS), 1);
	EXPECT_EQ(allocation.remaining_points(), 59);
	EXPECT_EQ(allocation.score(ThiefSkill::OPEN_LOCKS), before + 1);
}

// "No more than 30 points can be assigned to any single skill."
TEST(ThiefSkillsTest, AtFirstLevelNoSkillTakesMoreThanThirtyOfTheSixty)
{
	ThiefSkillAllocation allocation = first_level(NO_RACE, 13, ThiefArmor::LEATHER);

	for (int point = 0; point < 30; ++point)
	{
		ASSERT_TRUE(allocation.can_assign(ThiefSkill::FIND_REMOVE_TRAPS)) << point;
		allocation.assign(ThiefSkill::FIND_REMOVE_TRAPS);
	}

	EXPECT_EQ(allocation.assigned_to(ThiefSkill::FIND_REMOVE_TRAPS), 30);
	EXPECT_FALSE(allocation.can_assign(ThiefSkill::FIND_REMOVE_TRAPS));
	EXPECT_EQ(allocation.remaining_points(), 30);

	// The pool is untouched by the refusal, and another skill can still take it.
	EXPECT_TRUE(allocation.can_assign(ThiefSkill::MOVE_SILENTLY));
}

// "No more than 15 points per level can be assigned to a single skill" - and the
// limit is on this grant, not on what the skill already carries.
TEST(ThiefSkillsTest, ALaterGrantPutsNoMoreThanFifteenOnASkillAlreadyBought)
{
	constexpr std::array<int, THIEF_SKILL_COUNT> ALREADY{ 0, 30, 0, 0, 0, 0, 0, 0 };
	ThiefSkillAllocation allocation{
		ALREADY,
		thief_skill_grant_at_level(2),
		NO_RACE,
		13,
		ThiefArmor::LEATHER
	};

	for (int point = 0; point < 15; ++point)
	{
		ASSERT_TRUE(allocation.can_assign(ThiefSkill::OPEN_LOCKS)) << point;
		allocation.assign(ThiefSkill::OPEN_LOCKS);
	}

	EXPECT_FALSE(allocation.can_assign(ThiefSkill::OPEN_LOCKS));
	EXPECT_EQ(allocation.assigned_to(ThiefSkill::OPEN_LOCKS), 15);
	EXPECT_EQ(allocation.total_points().at(thief_skill_index(ThiefSkill::OPEN_LOCKS)), 45);
	EXPECT_EQ(allocation.score(ThiefSkill::OPEN_LOCKS), 55);
}

// A point that cannot raise the score is a point thrown away, so the screen
// refuses it: 95 is the ceiling "including all adjustments".
TEST(ThiefSkillsTest, NoPointIsTakenOnceTheScoreStandsAtNinetyFive)
{
	constexpr std::array<int, THIEF_SKILL_COUNT> ALREADY{ 0, 0, 0, 0, 0, 0, 25, 0 };
	ThiefSkillAllocation allocation{
		ALREADY,
		thief_skill_grant_at_level(2),
		NO_RACE,
		13,
		ThiefArmor::NONE
	};

	// Climb Walls: base 60, no armour +10, 25 already spent - the ceiling exactly.
	EXPECT_EQ(allocation.score(ThiefSkill::CLIMB_WALLS), 95);
	EXPECT_FALSE(allocation.can_assign(ThiefSkill::CLIMB_WALLS));
	EXPECT_EQ(allocation.remaining_points(), 30);
}

TEST(ThiefSkillsTest, APointComesBackAndEarlierLevelsPointsDoNot)
{
	constexpr std::array<int, THIEF_SKILL_COUNT> ALREADY{ 0, 0, 0, 7, 0, 0, 0, 0 };
	ThiefSkillAllocation allocation{
		ALREADY,
		thief_skill_grant_at_level(3),
		NO_RACE,
		13,
		ThiefArmor::LEATHER
	};

	EXPECT_FALSE(allocation.can_take_back(ThiefSkill::MOVE_SILENTLY));

	allocation.assign(ThiefSkill::MOVE_SILENTLY);
	EXPECT_TRUE(allocation.can_take_back(ThiefSkill::MOVE_SILENTLY));
	allocation.take_back(ThiefSkill::MOVE_SILENTLY);

	EXPECT_EQ(allocation.assigned_to(ThiefSkill::MOVE_SILENTLY), 0);
	EXPECT_EQ(allocation.remaining_points(), 30);
	EXPECT_EQ(allocation.total_points().at(thief_skill_index(ThiefSkill::MOVE_SILENTLY)), 7);
	EXPECT_FALSE(allocation.can_take_back(ThiefSkill::MOVE_SILENTLY));
}

// The race is carried into every score the screen shows, so a dwarf reads his
// +15 on traps and his -10 on walls without the player adding anything.
TEST(ThiefSkillsTest, TheAllocationCarriesTheRacialColumnIntoEveryScore)
{
	const ThiefSkillAllocation dwarf = first_level(DWARF, 13, ThiefArmor::LEATHER);
	EXPECT_EQ(dwarf.score(ThiefSkill::FIND_REMOVE_TRAPS), 20);
	EXPECT_EQ(dwarf.score(ThiefSkill::CLIMB_WALLS), 50);
	EXPECT_EQ(dwarf.score(ThiefSkill::READ_LANGUAGES), 0);

	const ThiefSkillAllocation halfling = first_level(HALFLING, 13, ThiefArmor::LEATHER);
	EXPECT_EQ(halfling.score(ThiefSkill::HIDE_IN_SHADOWS), 20);
	EXPECT_EQ(halfling.score(ThiefSkill::CLIMB_WALLS), 45);
}

TEST(ThiefSkillsTest, TheNamesAndTheCursorCoverAllEight)
{
	ThiefSkill walked = ThiefSkill::PICK_POCKETS;
	for (std::size_t step = 0; step < THIEF_SKILL_COUNT; ++step)
	{
		EXPECT_FALSE(thief_skill_name(walked).empty());
		walked = next_thief_skill(walked);
	}
	EXPECT_EQ(walked, ThiefSkill::PICK_POCKETS);

	EXPECT_EQ(previous_thief_skill(ThiefSkill::PICK_POCKETS), ThiefSkill::READ_LANGUAGES);
	EXPECT_EQ(next_thief_skill(ThiefSkill::READ_LANGUAGES), ThiefSkill::PICK_POCKETS);
}

// end of file: ThiefSkillsTest.cpp
