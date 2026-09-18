// Checks the Dexterity table the game loads against the Player's Handbook's Table 2.
//
// What it is for. Every Dexterity adjustment the game applies - the reaction adjustment
// to the surprise roll, the missile adjustment to a shot, the defensive adjustment to
// armour class - is read from src/json/dexterity.json, and that file is the only place
// the numbers live. Table 2 is on PDF pages 31-32 of the 2e archive; the rows below were
// generated from its text, not typed.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DexterityTableTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/DexterityAttributes.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"

namespace
{
// One row of Table 2: the score and its three adjustments.
struct TableTwoRow
{
	int score{ 0 };
	int reaction{ 0 };
	int missile{ 0 };
	int defensive{ 0 };
};

const std::vector<TableTwoRow> TABLE_TWO{
	{ 1, -6, -6, 5 },
	{ 2, -4, -4, 5 },
	{ 3, -3, -3, 4 },
	{ 4, -2, -2, 3 },
	{ 5, -1, -1, 2 },
	{ 6, 0, 0, 1 },
	{ 7, 0, 0, 0 },
	{ 8, 0, 0, 0 },
	{ 9, 0, 0, 0 },
	{ 10, 0, 0, 0 },
	{ 11, 0, 0, 0 },
	{ 12, 0, 0, 0 },
	{ 13, 0, 0, 0 },
	{ 14, 0, 0, 0 },
	{ 15, 0, 0, -1 },
	{ 16, 1, 1, -2 },
	{ 17, 2, 2, -3 },
	{ 18, 2, 2, -4 },
	{ 19, 3, 3, -4 },
	{ 20, 3, 3, -4 },
	{ 21, 4, 4, -5 },
	{ 22, 4, 4, -5 },
	{ 23, 4, 4, -5 },
	{ 24, 5, 5, -6 },
	{ 25, 5, 5, -6 },
};
} // namespace

class DexterityTableTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		ctx = game.context();
	}

	Game game;
	GameContext ctx;
};

// Every cell the game loads, for the scores it has, is the book's.
TEST_F(DexterityTableTest, EveryRowMatchesTableTwo)
{
	const std::vector<DexterityAttributes>& loaded = game.dataManager.get_dexterity_attributes();
	ASSERT_GE(loaded.size(), TABLE_TWO.size());

	for (const TableTwoRow& expected : TABLE_TWO)
	{
		const DexterityAttributes& row = loaded.at(expected.score - 1);
		EXPECT_EQ(row.Dex, expected.score);
		EXPECT_EQ(row.ReactionAdj, expected.reaction) << "reaction adjustment at Dexterity " << expected.score;
		EXPECT_EQ(row.MissileAttackAdj, expected.missile) << "missile adjustment at Dexterity " << expected.score;
		EXPECT_EQ(row.DefensiveAdj, expected.defensive) << "defensive adjustment at Dexterity " << expected.score;
	}
}

// No ability goes past 25 by the book; the game has no cap, and a Dexterity enhancement on
// equipment adds on top of gauntlets. A score beyond the table reads its last row.
TEST_F(DexterityTableTest, AScorePastTheTableReadsItsLastRow)
{
	const DexterityAttributes row = game.dataManager.dexterity_for(26);

	EXPECT_EQ(row.ReactionAdj, 5);
	EXPECT_EQ(row.MissileAttackAdj, 5);
	EXPECT_EQ(row.DefensiveAdj, -6);
}

// A score below 1 belongs only to a creature never given one, and carries no adjustment.
TEST_F(DexterityTableTest, AScoreBelowOneCarriesNoAdjustment)
{
	const DexterityAttributes row = game.dataManager.dexterity_for(0);

	EXPECT_EQ(row.ReactionAdj, 0);
	EXPECT_EQ(row.MissileAttackAdj, 0);
	EXPECT_EQ(row.DefensiveAdj, 0);
}

// Gauntlets taking an elf's 19 to 21 improve armour class further, -5 on the table.
TEST_F(DexterityTableTest, DexterityPastTwentyStillImprovesArmourClass)
{
	Creature elf{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "elf", 0 } };
	elf.experienceReward = std::make_unique<ExperienceReward>(0);
	elf.healthPool = std::make_unique<HealthPool>(10);
	elf.armorClass = std::make_unique<ArmorClass>(10);
	elf.set_dexterity(21);

	elf.update_armor_class(ctx);

	EXPECT_EQ(elf.get_armor_class(), 5) << "Dexterity 21 is -5 to armour class";
}

// Through the path that uses it: Dexterity 7 leaves armour class 10 where it is.
TEST_F(DexterityTableTest, DexteritySevenLeavesArmourClassAlone)
{
	Creature fighter{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "fighter", 0 } };
	fighter.experienceReward = std::make_unique<ExperienceReward>(0);
	fighter.healthPool = std::make_unique<HealthPool>(10);
	fighter.armorClass = std::make_unique<ArmorClass>(10);
	fighter.set_dexterity(7);

	fighter.update_armor_class(ctx);

	EXPECT_EQ(fighter.get_armor_class(), 10) << "Dexterity 7 carries no defensive adjustment";
}
