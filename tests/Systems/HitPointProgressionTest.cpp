// file: HitPointProgressionTest.cpp
// Pins the AD&D 2e hit point progression against the Player's Handbook tables,
// which state for each class the level hit dice stop at and the flat gain after.
//
// Expected values come from the book, not from the code under test:
//   Warrior, Table 14: "one 10-sided hit die per level from 1st through 9th.
//     After 9th level, warriors gain just 3 hit points per level"
//   Priest,  Table 23: one d8 through 9th, then 2 per level
//   Rogue,   Table 25: one d6 through 10th, then 2 per level
//   Wizard,  Table 20: one d4 through 10th, then 1 per level

#include <gtest/gtest.h>

#include "src/Actor/CreatureClass.h"
#include "src/Systems/LevelUpSystem.h"

// Warriors roll through 9th and gain 3 a level after.
TEST(HitPointProgressionTest, FighterMatchesTableFourteen)
{
	const auto progression = LevelUpSystem::hit_point_progression(CreatureClass::FIGHTER);

	EXPECT_EQ(progression.lastRolledLevel, 9);
	EXPECT_EQ(progression.flatGain, 3);
}

// Priests roll through 9th and gain 2 a level after.
TEST(HitPointProgressionTest, ClericMatchesTableTwentyThree)
{
	const auto progression = LevelUpSystem::hit_point_progression(CreatureClass::CLERIC);

	EXPECT_EQ(progression.lastRolledLevel, 9);
	EXPECT_EQ(progression.flatGain, 2);
}

// Rogues roll through 10th and gain 2 a level after.
TEST(HitPointProgressionTest, RogueMatchesTableTwentyFive)
{
	const auto progression = LevelUpSystem::hit_point_progression(CreatureClass::ROGUE);

	EXPECT_EQ(progression.lastRolledLevel, 10);
	EXPECT_EQ(progression.flatGain, 2);
}

// Wizards roll through 10th and gain 1 a level after.
TEST(HitPointProgressionTest, WizardMatchesTableTwenty)
{
	const auto progression = LevelUpSystem::hit_point_progression(CreatureClass::WIZARD);

	EXPECT_EQ(progression.lastRolledLevel, 10);
	EXPECT_EQ(progression.flatGain, 1);
}

// Monsters do not advance by these tables, but the lookup must still answer.
TEST(HitPointProgressionTest, MonsterHasADefinedProgression)
{
	const auto monster = LevelUpSystem::hit_point_progression(CreatureClass::MONSTER);
	const auto fighter = LevelUpSystem::hit_point_progression(CreatureClass::FIGHTER);

	EXPECT_EQ(monster.lastRolledLevel, fighter.lastRolledLevel);
	EXPECT_EQ(monster.flatGain, fighter.flatGain);
}

// No class rolls forever, and none gains nothing afterwards.
TEST(HitPointProgressionTest, EveryClassStopsRollingAndStillGains)
{
	for (const auto creatureClass : { CreatureClass::FIGHTER, CreatureClass::ROGUE,
			 CreatureClass::CLERIC, CreatureClass::WIZARD, CreatureClass::MONSTER })
	{
		const auto progression = LevelUpSystem::hit_point_progression(creatureClass);

		EXPECT_GT(progression.lastRolledLevel, 0);
		EXPECT_LE(progression.lastRolledLevel, 10) << "no 2e class rolls past 10th";
		EXPECT_GT(progression.flatGain, 0) << "a level must always be worth something";
	}
}

// The table above is only worth having if the level-up path reads it. These
// drive the public entry point, LevelUpSystem::apply_level_up_benefits.

#include "src/Actor/Creature.h"
#include "src/Combat/ExperienceReward.h"
#include "tests/mocks/MockGameContext.h"

class HitPointGainTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();

		fighter.healthPool = std::make_unique<HealthPool>(50);
		fighter.armorClass = std::make_unique<ArmorClass>(10);
		fighter.experienceReward = std::make_unique<ExperienceReward>(0);
		fighter.set_creature_class(CreatureClass::FIGHTER);

		// A high score, so a Constitution bonus would be obvious if applied.
		fighter.set_constitution(18);
	}

	int gain_at_level(int newLevel)
	{
		const int before = fighter.get_max_hp();
		LevelUpSystem::apply_level_up_benefits(fighter, newLevel, &ctx);
		return fighter.get_max_hp() - before;
	}

	MockGameContext mock{};
	GameContext ctx{};
	Creature fighter{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "hero", 0 } };
};

// Past 9th a warrior gains exactly 3, with no die and no Constitution bonus,
// however high the score.
TEST_F(HitPointGainTest, FighterPastNinthGainsExactlyThree)
{
	EXPECT_EQ(gain_at_level(10), 3);
	EXPECT_EQ(gain_at_level(11), 3);
	EXPECT_EQ(gain_at_level(20), 3);
}
