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
//
// And the Constitution bonus each rolled die carries, Table 3 (PDF pages 33-34 of the
// 2e archive): the warrior column above +2 for warriors only, +2 at most for every
// other class, a penalty never capped, and no die worth less than 1. That bonus is
// added to "each Hit Die rolled for the character" (PDF page 32): so on exactly the
// levels above, and on no level of a monster's. Table 3's footnotes raise the die
// itself from 20 up: "All 1s rolled for Hit Dice are automatically considered 2s"
// at 20, 1s and 2s count as 3s at 21-22, and 1s to 3s as 4s at 23-25.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HitPointProgressionTest.*:HitPointGainTest.*:ConstitutionAdjustmentLevelTest.*

#include <gtest/gtest.h>

#include <array>

#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/ExperienceReward.h"
#include "src/LevelUpSystem.h"
#include "tests/mocks/MockGameContext.h"

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

// The Constitution adjustment rides on each rolled die (page 32), so it holds through
// the last level a class rolls.
TEST(ConstitutionAdjustmentLevelTest, EachClassTakesItThroughItsLastRolledDie)
{
	struct LastRolledDie
	{
		CreatureClass creatureClass{ CreatureClass::MONSTER };
		int level{ 0 };
	};
	constexpr std::array<LastRolledDie, 4> FROM_THE_CLASS_TABLES{ {
		{ CreatureClass::FIGHTER, 9 },
		{ CreatureClass::CLERIC, 9 },
		{ CreatureClass::ROGUE, 10 },
		{ CreatureClass::WIZARD, 10 },
	} };

	for (const LastRolledDie& expected : FROM_THE_CLASS_TABLES)
	{
		EXPECT_TRUE(LevelUpSystem::takes_constitution_adjustment_at(expected.creatureClass, 1));
		EXPECT_TRUE(LevelUpSystem::takes_constitution_adjustment_at(expected.creatureClass, expected.level));
		EXPECT_FALSE(LevelUpSystem::takes_constitution_adjustment_at(expected.creatureClass, expected.level + 1));
	}
}

// A monster's levels are its hit dice and every one of them is rolled, so none
// of them is past the level a character's table stops rolling at.
TEST(ConstitutionAdjustmentLevelTest, AMonsterTakesItOnEveryLevel)
{
	for (int level = 1; level <= 13; ++level)
	{
		EXPECT_TRUE(LevelUpSystem::takes_constitution_adjustment_at(CreatureClass::MONSTER, level)) << "level " << level;
	}
}

// A change of score is multiplied by the levels that took the adjustment.
TEST(ConstitutionAdjustmentLevelTest, AChangeCountsOnlyTheLevelsThatTookIt)
{
	EXPECT_EQ(LevelUpSystem::levels_taking_constitution_adjustment(CreatureClass::WIZARD, 3), 3);
	EXPECT_EQ(LevelUpSystem::levels_taking_constitution_adjustment(CreatureClass::WIZARD, 12), 10);
	EXPECT_EQ(LevelUpSystem::levels_taking_constitution_adjustment(CreatureClass::ROGUE, 10), 10);
	EXPECT_EQ(LevelUpSystem::levels_taking_constitution_adjustment(CreatureClass::FIGHTER, 12), 9);
	EXPECT_EQ(LevelUpSystem::levels_taking_constitution_adjustment(CreatureClass::CLERIC, 9), 9);
	EXPECT_EQ(LevelUpSystem::levels_taking_constitution_adjustment(CreatureClass::MONSTER, 13), 13);
}

// The table above is only worth having if the level-up path reads it. These
// drive the public entry point, LevelUpSystem::apply_level_up_benefits.

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

	// The hit points 2nd level adds to a new creature of a class and Constitution
	// score, its hit die scripted to roll `rolled`.
	int gain_for(CreatureClass creatureClass, int constitution, int rolled)
	{
		Creature adventurer{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "adventurer", 0 } };
		adventurer.healthPool = std::make_unique<HealthPool>(50);
		adventurer.armorClass = std::make_unique<ArmorClass>(10);
		adventurer.experienceReward = std::make_unique<ExperienceReward>(0);
		adventurer.set_creature_class(creatureClass);
		adventurer.set_constitution(constitution);
		mock.dice.set_next_roll(rolled);

		const int before = adventurer.get_max_hp();
		LevelUpSystem::apply_level_up_benefits(adventurer, 2, &ctx);
		return adventurer.get_max_hp() - before;
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

// Table 3's footnote, read directly: "Parenthetical bonus applies to warriors only. All
// other classes receive maximum bonus of +2 per die."
TEST_F(HitPointGainTest, TheBonusAboveTwoIsTheWarriorsAlone)
{
	EXPECT_EQ(mock.data_manager.constitution_hit_point_adjustment(17, CreatureClass::FIGHTER), 3);
	EXPECT_EQ(mock.data_manager.constitution_hit_point_adjustment(17, CreatureClass::WIZARD), 2);
	EXPECT_EQ(mock.data_manager.constitution_hit_point_adjustment(3, CreatureClass::WIZARD), -2);
}

// Through a level: a rolled 4 is worth 6 to a rogue, priest or wizard at any score from
// 16 up. A 4 is the highest die any footnote raises to, so only the bonus moves it.
TEST_F(HitPointGainTest, ANonWarriorsBonusStopsAtTwo)
{
	for (const CreatureClass creatureClass : { CreatureClass::ROGUE, CreatureClass::CLERIC, CreatureClass::WIZARD })
	{
		for (const int constitution : { 16, 17, 18, 19, 20, 25 })
		{
			EXPECT_EQ(gain_for(creatureClass, constitution, 4), 6) << "class " << static_cast<int>(creatureClass) << " at Constitution " << constitution;
		}
	}
}

// A warrior takes the parenthetical bonus, Table 3's warrior column, on a rolled 4 that
// no footnote raises.
TEST_F(HitPointGainTest, AWarriorGainsTheParentheticalBonus)
{
	struct WarriorBonus
	{
		int constitution{ 0 };
		int bonus{ 0 };
	};
	constexpr std::array<WarriorBonus, 6> TABLE_THREE_WARRIOR{ {
		{ 17, 3 },
		{ 18, 4 },
		{ 19, 5 },
		{ 20, 5 },
		{ 21, 6 },
		{ 25, 7 },
	} };

	for (const WarriorBonus& expected : TABLE_THREE_WARRIOR)
	{
		EXPECT_EQ(gain_for(CreatureClass::FIGHTER, expected.constitution, 4), 4 + expected.bonus) << "Constitution " << expected.constitution;
	}
}

// Only a bonus is capped. Constitution 3's -2 takes a wizard's rolled 4 to 2, and since
// "no Hit Die ever yields less than 1 hit point" (page 32), a rolled 2 at Constitution
// 1's -3 still yields 1.
TEST_F(HitPointGainTest, APenaltyIsNotCappedAndADieYieldsAtLeastOne)
{
	EXPECT_EQ(gain_for(CreatureClass::WIZARD, 3, 4), 2);
	EXPECT_EQ(gain_for(CreatureClass::WIZARD, 1, 2), 1);
}

// A monster rolls its die like a warrior and takes the warrior's bonus on it, as
// when it was made: a rolled 3 at Constitution 18 is worth 3 and 4. The
// non-warrior cap would make it 5.
TEST_F(HitPointGainTest, AMonstersLevelAddsItsConstitutionLikeAWarriors)
{
	EXPECT_EQ(gain_for(CreatureClass::MONSTER, 18, 3), 7);
}

// Table 3's footnotes, row by row: nothing below 20, then 2, 3, 3, 4, 4, 4.
TEST_F(HitPointGainTest, TableThreeFootnotesSetTheLowestDie)
{
	for (int score = 1; score <= 19; ++score)
	{
		EXPECT_EQ(mock.data_manager.constitution_for(score).hitDieMinimum, 1) << "Constitution " << score;
	}
	EXPECT_EQ(mock.data_manager.constitution_for(20).hitDieMinimum, 2);
	EXPECT_EQ(mock.data_manager.constitution_for(21).hitDieMinimum, 3);
	EXPECT_EQ(mock.data_manager.constitution_for(22).hitDieMinimum, 3);
	EXPECT_EQ(mock.data_manager.constitution_for(23).hitDieMinimum, 4);
	EXPECT_EQ(mock.data_manager.constitution_for(24).hitDieMinimum, 4);
	EXPECT_EQ(mock.data_manager.constitution_for(25).hitDieMinimum, 4);
}

// Through a level: a low roll counts as the footnote's number, then takes the bonus.
TEST_F(HitPointGainTest, FromTwentyALowRollCountsAsTheFootnoteSays)
{
	struct LowRoll
	{
		CreatureClass creatureClass{ CreatureClass::MONSTER };
		int constitution{ 0 };
		int rolled{ 0 };
		int gain{ 0 };
	};
	constexpr std::array<LowRoll, 7> RAISED_BY_TABLE_THREE{ {
		{ CreatureClass::FIGHTER, 20, 1, 2 + 5 },
		{ CreatureClass::WIZARD, 20, 1, 2 + 2 },
		{ CreatureClass::FIGHTER, 21, 2, 3 + 6 },
		{ CreatureClass::FIGHTER, 22, 1, 3 + 6 },
		{ CreatureClass::FIGHTER, 23, 3, 4 + 6 },
		{ CreatureClass::CLERIC, 24, 2, 4 + 2 },
		{ CreatureClass::FIGHTER, 25, 1, 4 + 7 },
	} };

	for (const LowRoll& expected : RAISED_BY_TABLE_THREE)
	{
		EXPECT_EQ(gain_for(expected.creatureClass, expected.constitution, expected.rolled), expected.gain)
			<< "class " << static_cast<int>(expected.creatureClass) << " at Constitution " << expected.constitution << " rolling " << expected.rolled;
	}
}

// Only a low roll is raised: a 5 at Constitution 20 stays 5, and at 19 a 1 stays 1.
TEST_F(HitPointGainTest, TheMinimumRaisesOnlyALowRoll)
{
	EXPECT_EQ(gain_for(CreatureClass::FIGHTER, 20, 5), 5 + 5);
	EXPECT_EQ(gain_for(CreatureClass::FIGHTER, 19, 1), 1 + 5);
}

// A monster reads the same footnotes: at Constitution 25 a rolled 1 counts as 4
// and then takes the warrior's 7.
TEST_F(HitPointGainTest, AMonstersDieIsRaisedLikeAWarriors)
{
	EXPECT_EQ(gain_for(CreatureClass::MONSTER, 25, 1), 11);
}

// Past 9th a warrior rolls no die, so there is nothing for Constitution 25 to raise.
TEST_F(HitPointGainTest, AFlatGainIsNotRaised)
{
	fighter.set_constitution(25);

	EXPECT_EQ(gain_at_level(10), 3);
}
