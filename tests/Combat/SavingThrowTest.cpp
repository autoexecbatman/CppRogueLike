// file: SavingThrowTest.cpp
// Saving throws against AD&D 2nd edition Player's Handbook Table 60. Every
// number below is read from the printed table, never from what the code
// returned.
//
// The table is banded, and the bands are what decides when a save improves:
//
//   Warriors  0, 1-2, 3-4, 5-6, 7-8, 9-10, 11-12, 13-14, 15-16, 17+
//   Priests   1-3, 4-6, 7-9, 10-12, 13-15, 16-18, 19+
//   Rogues    1-4, 5-8, 9-12, 13-16, 17-20, 21+
//   Wizards   1-5, 6-10, 11-15, 16-20, 21+
//
// The defect this pins: the level-up announced "Saving throws improved!" from a
// hand-written list of levels that matched no band. Priests, rogues and wizards
// were told one level early every time; warriors were told on 6 and 12, which
// improve nothing, and not on 5, 7, 11, 13 or 17, which do.

#include <gtest/gtest.h>

#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/ExperienceReward.h"
#include "src/SavingThrow.h"
#include "src/DataManager.h"
#include "src/LevelUpSystem.h"
#include "tests/mocks/MockGameContext.h"

// ---------------------------------------------------------------------------
// The table itself
// ---------------------------------------------------------------------------

// The first row of each class, which is where every new character sits.
TEST(SavingThrowTableTest, FirstLevelMatchesTheBook)
{
	EXPECT_EQ(SavingThrows::target(CreatureClass::FIGHTER, 1, SavingThrow::PARALYZATION_POISON_DEATH), 14);
	EXPECT_EQ(SavingThrows::target(CreatureClass::FIGHTER, 1, SavingThrow::ROD_STAFF_WAND), 16);
	EXPECT_EQ(SavingThrows::target(CreatureClass::FIGHTER, 1, SavingThrow::PETRIFICATION_POLYMORPH), 15);
	EXPECT_EQ(SavingThrows::target(CreatureClass::FIGHTER, 1, SavingThrow::BREATH_WEAPON), 17);
	EXPECT_EQ(SavingThrows::target(CreatureClass::FIGHTER, 1, SavingThrow::SPELL), 17);

	EXPECT_EQ(SavingThrows::target(CreatureClass::CLERIC, 1, SavingThrow::PARALYZATION_POISON_DEATH), 10);
	EXPECT_EQ(SavingThrows::target(CreatureClass::CLERIC, 1, SavingThrow::SPELL), 15);

	EXPECT_EQ(SavingThrows::target(CreatureClass::ROGUE, 1, SavingThrow::PARALYZATION_POISON_DEATH), 13);
	EXPECT_EQ(SavingThrows::target(CreatureClass::ROGUE, 1, SavingThrow::SPELL), 15);

	EXPECT_EQ(SavingThrows::target(CreatureClass::WIZARD, 1, SavingThrow::PARALYZATION_POISON_DEATH), 14);
	EXPECT_EQ(SavingThrows::target(CreatureClass::WIZARD, 1, SavingThrow::SPELL), 12);
}

// A wizard resists spells best at first level and a priest resists death best:
// the columns are not the same shape, so a single flat number cannot stand in.
TEST(SavingThrowTableTest, TheColumnsDifferByClass)
{
	EXPECT_LT(SavingThrows::target(CreatureClass::WIZARD, 1, SavingThrow::SPELL),
		SavingThrows::target(CreatureClass::FIGHTER, 1, SavingThrow::SPELL));
	EXPECT_LT(SavingThrows::target(CreatureClass::CLERIC, 1, SavingThrow::PARALYZATION_POISON_DEATH),
		SavingThrows::target(CreatureClass::WIZARD, 1, SavingThrow::PARALYZATION_POISON_DEATH));
}

// The last row of each class, so the table is pinned at both ends.
TEST(SavingThrowTableTest, TheHighestRowMatchesTheBook)
{
	EXPECT_EQ(SavingThrows::target(CreatureClass::FIGHTER, 17, SavingThrow::PARALYZATION_POISON_DEATH), 3);
	EXPECT_EQ(SavingThrows::target(CreatureClass::FIGHTER, 30, SavingThrow::SPELL), 6);
	EXPECT_EQ(SavingThrows::target(CreatureClass::CLERIC, 19, SavingThrow::PARALYZATION_POISON_DEATH), 2);
	EXPECT_EQ(SavingThrows::target(CreatureClass::ROGUE, 21, SavingThrow::ROD_STAFF_WAND), 4);
	EXPECT_EQ(SavingThrows::target(CreatureClass::WIZARD, 21, SavingThrow::SPELL), 4);
}

// Warriors have a 0-level row, and nothing below the table falls off it.
TEST(SavingThrowTableTest, ALevelBelowTheTableTakesItsFirstRow)
{
	EXPECT_EQ(SavingThrows::target(CreatureClass::FIGHTER, 0, SavingThrow::SPELL), 19);
	EXPECT_EQ(SavingThrows::target(CreatureClass::WIZARD, 0, SavingThrow::SPELL), 12);
	EXPECT_EQ(SavingThrows::target(CreatureClass::WIZARD, -3, SavingThrow::SPELL), 12);
}

// Monsters save on the warrior rows, as they attack on the warrior table.
TEST(SavingThrowTableTest, MonstersSaveOnTheWarriorRows)
{
	for (int level = 1; level <= 20; ++level)
	{
		EXPECT_EQ(SavingThrows::target(CreatureClass::MONSTER, level, SavingThrow::SPELL),
			SavingThrows::target(CreatureClass::FIGHTER, level, SavingThrow::SPELL))
			<< "level " << level;
	}
}

// Every band start improves the save, and no level inside a band does.
TEST(SavingThrowTableTest, ImprovementFollowsTheBands)
{
	const std::vector<int> warrior{ 3, 5, 7, 9, 11, 13, 15, 17 };
	const std::vector<int> priest{ 4, 7, 10, 13, 16, 19 };
	const std::vector<int> rogue{ 5, 9, 13, 17, 21 };
	const std::vector<int> wizard{ 6, 11, 16, 21 };

	const std::vector<std::pair<CreatureClass, std::vector<int>>> expected{
		{ CreatureClass::FIGHTER, warrior },
		{ CreatureClass::CLERIC, priest },
		{ CreatureClass::ROGUE, rogue },
		{ CreatureClass::WIZARD, wizard },
	};

	for (const auto& [creatureClass, improvingLevels] : expected)
	{
		for (int level = 2; level <= 21; ++level)
		{
			const bool shouldImprove =
				std::find(improvingLevels.begin(), improvingLevels.end(), level) != improvingLevels.end();
			EXPECT_EQ(SavingThrows::improves_at(creatureClass, level), shouldImprove)
				<< "class " << static_cast<int>(creatureClass) << " level " << level;
		}
	}
}

// ---------------------------------------------------------------------------
// What the level-up tells the player
// ---------------------------------------------------------------------------

class SavingThrowLevelUpTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// The hit point gain reads the constitution table, so the level-up path
		// needs a loaded data manager.
		dataManager.load_all_data(mock.messages);
		ctx = mock.to_game_context();
		ctx.dataManager = &dataManager;
	}

	// Levels up a fresh creature of the class and reports whether it was told
	// its saving throws improved.
	bool announced_at(CreatureClass creatureClass, int newLevel)
	{
		Creature creature{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "hero", 0 } };
		creature.healthPool = std::make_unique<HealthPool>(50);
		creature.armorClass = std::make_unique<ArmorClass>(10);
		creature.experienceReward = std::make_unique<ExperienceReward>(0);
		creature.set_creature_class(creatureClass);
		creature.set_creature_level(newLevel);

		const size_t before = mock.messages.get_stored_message_count();
		LevelUpSystem::apply_level_up_benefits(creature, newLevel, &ctx);

		for (size_t index = before; index < mock.messages.get_stored_message_count(); ++index)
		{
			for (const auto& part : mock.messages.get_attack_message_at(index))
			{
				if (part.logMessageText.find("Saving throws improved") != std::string::npos)
				{
					return true;
				}
			}
		}
		return false;
	}

	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
};

// A wizard's rows begin at 1, 6, 11, 16: those are the levels that improve.
TEST_F(SavingThrowLevelUpTest, AWizardIsToldOnTheLevelsTheTableMovesOn)
{
	EXPECT_TRUE(announced_at(CreatureClass::WIZARD, 6));
	EXPECT_FALSE(announced_at(CreatureClass::WIZARD, 5)) << "5 is inside the 1-5 row";
	EXPECT_TRUE(announced_at(CreatureClass::WIZARD, 11));
	EXPECT_FALSE(announced_at(CreatureClass::WIZARD, 10)) << "10 is inside the 6-10 row";
}

// A priest's rows begin at 1, 4, 7, 10.
TEST_F(SavingThrowLevelUpTest, APriestIsToldOnTheLevelsTheTableMovesOn)
{
	EXPECT_TRUE(announced_at(CreatureClass::CLERIC, 4));
	EXPECT_FALSE(announced_at(CreatureClass::CLERIC, 3)) << "3 is inside the 1-3 row";
	EXPECT_TRUE(announced_at(CreatureClass::CLERIC, 7));
}

// A warrior improves every second level from 3, so 6 and 12 improve nothing.
TEST_F(SavingThrowLevelUpTest, AWarriorIsToldOnTheLevelsTheTableMovesOn)
{
	EXPECT_TRUE(announced_at(CreatureClass::FIGHTER, 3));
	EXPECT_TRUE(announced_at(CreatureClass::FIGHTER, 5));
	EXPECT_FALSE(announced_at(CreatureClass::FIGHTER, 6)) << "6 is inside the 5-6 row";
	EXPECT_TRUE(announced_at(CreatureClass::FIGHTER, 7));
}

// A rogue's rows begin at 1, 5, 9, 13.
TEST_F(SavingThrowLevelUpTest, ARogueIsToldOnTheLevelsTheTableMovesOn)
{
	EXPECT_TRUE(announced_at(CreatureClass::ROGUE, 5));
	EXPECT_FALSE(announced_at(CreatureClass::ROGUE, 4)) << "4 is inside the 1-4 row";
	EXPECT_TRUE(announced_at(CreatureClass::ROGUE, 9));
}
