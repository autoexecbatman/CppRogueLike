// file: Thac0ProgressionTest.cpp
// Pins the AD&D 2e attack-roll progression, and the question the level-up screen
// asks of it: did reaching this level actually move the character down the table.
//
// Expected values come from the Player's Handbook attack tables, not from the
// code under test:
//   Warrior, Table 24: THAC0 improves one point every level
//   Rogue,   Table 24: one point every two levels
//   Priest,  Table 24: two points every three levels
//   Wizard,  Table 24: one point every three levels

#include <gtest/gtest.h>

#include "src/CreatureClass.h"
#include "src/LevelUpSystem.h"

// A warrior starts at 20 and gains a point of it every level.
TEST(Thac0ProgressionTest, FighterImprovesEveryLevel)
{
	EXPECT_EQ(LevelUpSystem::thac0_for_level(CreatureClass::FIGHTER, 1), 20);
	EXPECT_EQ(LevelUpSystem::thac0_for_level(CreatureClass::FIGHTER, 2), 19);
	EXPECT_EQ(LevelUpSystem::thac0_for_level(CreatureClass::FIGHTER, 10), 11);
}

// A wizard holds 20 through third level and first improves at fourth.
TEST(Thac0ProgressionTest, WizardImprovesEveryThirdLevel)
{
	EXPECT_EQ(LevelUpSystem::thac0_for_level(CreatureClass::WIZARD, 2), 20);
	EXPECT_EQ(LevelUpSystem::thac0_for_level(CreatureClass::WIZARD, 3), 20);
	EXPECT_EQ(LevelUpSystem::thac0_for_level(CreatureClass::WIZARD, 4), 19);
}

// Monsters attack on the warrior table rather than having one of their own.
TEST(Thac0ProgressionTest, MonsterAttacksOnTheWarriorTable)
{
	for (int level = 1; level <= 20; ++level)
	{
		EXPECT_EQ(
			LevelUpSystem::thac0_for_level(CreatureClass::MONSTER, level),
			LevelUpSystem::thac0_for_level(CreatureClass::FIGHTER, level))
			<< "level " << level;
	}
}

// The lookup is total: a level off either end of the table still answers, with
// the worst THAC0 the tables hold.
TEST(Thac0ProgressionTest, LevelOutsideTheTableAnswersTwenty)
{
	EXPECT_EQ(LevelUpSystem::thac0_for_level(CreatureClass::FIGHTER, 0), 20);
	EXPECT_EQ(LevelUpSystem::thac0_for_level(CreatureClass::FIGHTER, 21), 20);
}

// Every level is an improvement for a warrior, which is what its table says.
TEST(Thac0ProgressionTest, EveryFighterLevelIsAnImprovement)
{
	for (int level = 2; level <= 20; ++level)
	{
		EXPECT_TRUE(LevelUpSystem::thac0_improves_at(CreatureClass::FIGHTER, level))
			<< "level " << level;
	}
}

// The levels a wizard's table does not move on. This is the case the level-up
// screen got wrong: it announced an improvement at every level of every class.
TEST(Thac0ProgressionTest, WizardDoesNotImproveOnTheLevelsBetween)
{
	EXPECT_FALSE(LevelUpSystem::thac0_improves_at(CreatureClass::WIZARD, 2));
	EXPECT_FALSE(LevelUpSystem::thac0_improves_at(CreatureClass::WIZARD, 3));
	EXPECT_TRUE(LevelUpSystem::thac0_improves_at(CreatureClass::WIZARD, 4));
	EXPECT_FALSE(LevelUpSystem::thac0_improves_at(CreatureClass::WIZARD, 5));
	EXPECT_FALSE(LevelUpSystem::thac0_improves_at(CreatureClass::WIZARD, 6));
	EXPECT_TRUE(LevelUpSystem::thac0_improves_at(CreatureClass::WIZARD, 7));
}

// A priest moves on the same levels as a wizard and by twice as much.
TEST(Thac0ProgressionTest, ClericImprovesTwoPointsEveryThirdLevel)
{
	EXPECT_FALSE(LevelUpSystem::thac0_improves_at(CreatureClass::CLERIC, 3));
	EXPECT_TRUE(LevelUpSystem::thac0_improves_at(CreatureClass::CLERIC, 4));

	EXPECT_EQ(
		LevelUpSystem::thac0_for_level(CreatureClass::CLERIC, 3)
			- LevelUpSystem::thac0_for_level(CreatureClass::CLERIC, 4),
		2);
}

// A rogue moves on every second level.
TEST(Thac0ProgressionTest, RogueImprovesEverySecondLevel)
{
	EXPECT_FALSE(LevelUpSystem::thac0_improves_at(CreatureClass::ROGUE, 2));
	EXPECT_TRUE(LevelUpSystem::thac0_improves_at(CreatureClass::ROGUE, 3));
	EXPECT_FALSE(LevelUpSystem::thac0_improves_at(CreatureClass::ROGUE, 4));
	EXPECT_TRUE(LevelUpSystem::thac0_improves_at(CreatureClass::ROGUE, 5));
}

// First level is where a character starts, so nothing improved to reach it.
TEST(Thac0ProgressionTest, FirstLevelIsNotAnImprovement)
{
	for (const auto creatureClass : { CreatureClass::FIGHTER, CreatureClass::ROGUE,
			 CreatureClass::CLERIC, CreatureClass::WIZARD, CreatureClass::MONSTER })
	{
		EXPECT_FALSE(LevelUpSystem::thac0_improves_at(creatureClass, 1));
	}
}
