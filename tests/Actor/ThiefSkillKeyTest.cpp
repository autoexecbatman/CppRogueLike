// file: ThiefSkillKeyTest.cpp
// What one key does on the thief skill screen, checked without opening the screen.
//
// after_thief_skill_key is the screen's key handling lifted out of
// MenuThiefSkills::menu, which also draws and so needs a window. The numbers come from
// the book: a 1st-level thief has 60 points with at most 30 on one skill, a later
// level 30 with at most 15, and no skill rises past 95 (Player's Handbook, Tables 26
// to 29, through ThiefSkillAllocation). The drawing stays covered by screenshots.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ThiefSkillKeyTest.*

#include <gtest/gtest.h>

#include <array>

#include "src/InputSystem.h"
#include "src/MenuThiefSkills.h"
#include "src/ThiefSkills.h"

namespace
{
// Table 27 for a human is a row of zeroes, and Dexterity 13 sits in Table 28's flat
// row, so neither moves a score here.
constexpr std::array<int, THIEF_SKILL_COUNT> NO_RACIAL_ADJUSTMENT{};
constexpr int UNADJUSTED_DEXTERITY = 13;
constexpr std::array<int, THIEF_SKILL_COUNT> NOTHING_SPENT_YET{};

// A thief at the given level with the screen opened on Pick Pockets. Leather is
// Table 29's column of zeroes, the armour the base scores already assume.
ThiefSkillScreenState screen_at_level(int level, std::array<int, THIEF_SKILL_COUNT> alreadySpent)
{
	return ThiefSkillScreenState{
		.cursor = ThiefSkill::PICK_POCKETS,
		.allocation = ThiefSkillAllocation{
			alreadySpent,
			thief_skill_grant_at_level(level),
			NO_RACIAL_ADJUSTMENT,
			UNADJUSTED_DEXTERITY,
			ThiefArmor::LEATHER }
	};
}

ThiefSkillScreenState first_level_screen()
{
	return screen_at_level(1, NOTHING_SPENT_YET);
}
} // namespace

TEST(ThiefSkillKeyTest, DownMovesTheCursorToTheNextSkill)
{
	const ThiefSkillScreenState after = after_thief_skill_key(GameKey::DOWN, first_level_screen());

	EXPECT_EQ(after.cursor, ThiefSkill::OPEN_LOCKS);
}

TEST(ThiefSkillKeyTest, UpFromTheFirstSkillWrapsToTheLast)
{
	const ThiefSkillScreenState after = after_thief_skill_key(GameKey::UP, first_level_screen());

	EXPECT_EQ(after.cursor, ThiefSkill::READ_LANGUAGES);
}

TEST(ThiefSkillKeyTest, RightPutsOnePointOnTheSkillUnderTheCursor)
{
	const ThiefSkillScreenState after = after_thief_skill_key(GameKey::RIGHT, first_level_screen());

	EXPECT_EQ(after.allocation.assigned_to(ThiefSkill::PICK_POCKETS), 1);
	EXPECT_EQ(after.allocation.remaining_points(), 59);
	EXPECT_EQ(after.allocation.assigned_to(ThiefSkill::OPEN_LOCKS), 0) << "the point went somewhere the cursor is not";
}

TEST(ThiefSkillKeyTest, LeftTakesThePointBack)
{
	const ThiefSkillScreenState placed = after_thief_skill_key(GameKey::RIGHT, first_level_screen());
	const ThiefSkillScreenState after = after_thief_skill_key(GameKey::LEFT, placed);

	EXPECT_EQ(after.allocation.assigned_to(ThiefSkill::PICK_POCKETS), 0);
	EXPECT_EQ(after.allocation.remaining_points(), 60);
}

TEST(ThiefSkillKeyTest, LeftWithNothingPlacedIsRefused)
{
	const ThiefSkillScreenState after = after_thief_skill_key(GameKey::LEFT, first_level_screen());

	EXPECT_EQ(after.allocation.assigned_to(ThiefSkill::PICK_POCKETS), 0);
	EXPECT_EQ(after.allocation.remaining_points(), 60) << "a point came back that was never placed";
}

// 30 is a 1st-level thief's limit on one skill.
TEST(ThiefSkillKeyTest, SpaceFillsTheSkillToThePerSkillLimit)
{
	const ThiefSkillScreenState after = after_thief_skill_key(GameKey::SPACE, first_level_screen());

	EXPECT_EQ(after.allocation.assigned_to(ThiefSkill::PICK_POCKETS), 30);
	EXPECT_EQ(after.allocation.remaining_points(), 30);
}

TEST(ThiefSkillKeyTest, RightAtThePerSkillLimitIsRefused)
{
	const ThiefSkillScreenState filled = after_thief_skill_key(GameKey::SPACE, first_level_screen());
	const ThiefSkillScreenState after = after_thief_skill_key(GameKey::RIGHT, filled);

	EXPECT_EQ(after.allocation.assigned_to(ThiefSkill::PICK_POCKETS), 30);
	EXPECT_EQ(after.allocation.remaining_points(), 30);
}

// Two skills filled to 30 spend all 60, so the third takes nothing.
TEST(ThiefSkillKeyTest, RightWithThePoolSpentIsRefused)
{
	ThiefSkillScreenState screen = after_thief_skill_key(GameKey::SPACE, first_level_screen());
	screen = after_thief_skill_key(GameKey::DOWN, screen);
	screen = after_thief_skill_key(GameKey::SPACE, screen);
	ASSERT_EQ(screen.allocation.remaining_points(), 0);
	screen = after_thief_skill_key(GameKey::DOWN, screen);

	const ThiefSkillScreenState after = after_thief_skill_key(GameKey::RIGHT, screen);

	EXPECT_EQ(after.allocation.assigned_to(ThiefSkill::FIND_REMOVE_TRAPS), 0);
	EXPECT_EQ(after.allocation.remaining_points(), 0);
}

// Climb Walls is based at 60. With 30 already spent at earlier levels it stands at 90,
// so at 4th level, limit 15, filling it stops at 95 and spends only 5.
TEST(ThiefSkillKeyTest, SpaceStopsAtTheCeilingOf95)
{
	std::array<int, THIEF_SKILL_COUNT> spentAtEarlierLevels{};
	spentAtEarlierLevels.at(thief_skill_index(ThiefSkill::CLIMB_WALLS)) = 30;
	ThiefSkillScreenState screen = screen_at_level(4, spentAtEarlierLevels);
	screen.cursor = ThiefSkill::CLIMB_WALLS;

	const ThiefSkillScreenState after = after_thief_skill_key(GameKey::SPACE, screen);

	EXPECT_EQ(after.allocation.score(ThiefSkill::CLIMB_WALLS), 95);
	EXPECT_EQ(after.allocation.assigned_to(ThiefSkill::CLIMB_WALLS), 5);
	EXPECT_EQ(after.allocation.remaining_points(), 25);
}

// Enter accepts the screen, which the menu does itself; here it is a key like any
// other the screen's state does not answer to.
TEST(ThiefSkillKeyTest, AKeyTheScreenDoesNotUseChangesNothing)
{
	for (const GameKey key : { GameKey::ENTER, GameKey::ESCAPE, GameKey::NONE })
	{
		const ThiefSkillScreenState after = after_thief_skill_key(key, first_level_screen());

		EXPECT_EQ(after.cursor, ThiefSkill::PICK_POCKETS);
		EXPECT_EQ(after.allocation.remaining_points(), 60);
		EXPECT_EQ(after.allocation.assigned_to(ThiefSkill::PICK_POCKETS), 0);
	}
}

// end of file: ThiefSkillKeyTest.cpp
