// file: MimicSpiderShopkeeperAbilityTest.cpp
// The mimic, the three spiders and the shopkeeper roll all six ability scores on the
// Dungeon Master Guide's Tables 6 to 11, read by the size and Intelligence rating their
// records in monsters.json give:
//
//   - mimic: large, Semi- (Monstrous Manual p1700, the killer mimic's column)
//   - small spider: tiny, Low (p1876, the hairy spider)
//   - giant spider: man-sized, Animal (p1876, the huge spider)
//   - web spinner: large, Low (p1876, the giant spider)
//   - shopkeeper: man-sized, Average (no entry; a human, by the driver's ruling)
//
// Each die is scripted to its highest, so every score is the 18 row of its table, read
// from the PDF pages 4857-4859. The mimic's Strength is also scripted to 13 to show an
// 18/xx score lands with its percentile.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MimicSpiderShopkeeperAbilityTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>

#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/RandomDice.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// More rolls than building any of these makes, the shopkeeper's shop stock included, so
// none falls back to a random one.
constexpr int SCRIPTED_ROLLS = 400;

// The six scores the tables give, Strength as its score and percentile.
struct AbilityScores
{
	int strength{ 0 };
	int exceptionalStrength{ 0 };
	int dexterity{ 0 };
	int constitution{ 0 };
	int intelligence{ 0 };
	int wisdom{ 0 };
	int charisma{ 0 };
};

void expect_scores(const Creature& creature, const AbilityScores& printed)
{
	EXPECT_EQ(creature.get_strength(), printed.strength);
	EXPECT_EQ(creature.get_exceptional_strength(), printed.exceptionalStrength);
	EXPECT_EQ(creature.get_dexterity(), printed.dexterity);
	EXPECT_EQ(creature.get_constitution(), printed.constitution);
	EXPECT_EQ(creature.get_intelligence(), printed.intelligence);
	EXPECT_EQ(creature.get_wisdom(), printed.wisdom);
	EXPECT_EQ(creature.get_charisma(), printed.charisma);
}
} // namespace

class MimicSpiderShopkeeperAbilityTest : public ::testing::Test
{
protected:
	void SetUp() override { ctx = mock.to_game_context(); }

	// Every die from here on at its highest.
	void script_highest()
	{
		for (int roll = 0; roll < SCRIPTED_ROLLS; ++roll)
		{
			mock.dice.set_next_roll(RandomDice::HIGHEST);
		}
	}

	// The monster built from its record, as the spawn table builds one.
	std::unique_ptr<Creature> build(std::string_view key)
	{
		return MonsterCreator::create_from_params(
			Vector2D{ 0, 0 },
			mock.monsterRegistry.get_params(key),
			ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
};

// Table 6 large 20, 7 large 19, 8 large 21; Tables 9 to 11 Semi- 6, 6, 6.
TEST_F(
	MimicSpiderShopkeeperAbilityTest,
	AMimicReadsTheLargeAndSemiIntelligentColumns)
{
	script_highest();
	const auto mimic = build("mimic");

	expect_scores(*mimic, { 20, 0, 19, 21, 6, 6, 6 });
}

// A 13 on Table 6 for a large creature is 18/51.
TEST_F(MimicSpiderShopkeeperAbilityTest, AMimicCarriesExceptionalStrength)
{
	for (const int die : { 6, 6, 1 })
	{
		mock.dice.set_next_roll(die);
	}
	script_highest();
	const auto mimic = build("mimic");

	EXPECT_EQ(mimic->get_strength(), 18);
	EXPECT_EQ(mimic->get_exceptional_strength(), 51);
}

// The hairy spider: tiny 15, 25, 15; Low 9, 9, 9.
TEST_F(MimicSpiderShopkeeperAbilityTest, ASmallSpiderReadsTheTinyAndLowColumns)
{
	script_highest();
	const auto spider = build("spider_small");

	expect_scores(*spider, { 15, 0, 25, 15, 9, 9, 9 });
}

// The huge spider: man-sized 18, 21, 19; Animal 4, 5, 3.
TEST_F(
	MimicSpiderShopkeeperAbilityTest,
	AGiantSpiderReadsTheManSizedAndAnimalColumns)
{
	script_highest();
	const auto spider = build("spider_giant");

	expect_scores(*spider, { 18, 0, 21, 19, 4, 5, 3 });
}

// The giant spider: large 20, 19, 21; Low 9, 9, 9.
TEST_F(MimicSpiderShopkeeperAbilityTest, AWebSpinnerReadsTheLargeAndLowColumns)
{
	script_highest();
	const auto spider = build("spider_weaver");

	expect_scores(*spider, { 20, 0, 19, 21, 9, 9, 9 });
}

// A human: man-sized 18, 21, 19; Average 13, 12, 12.
TEST_F(
	MimicSpiderShopkeeperAbilityTest,
	AShopkeeperReadsTheManSizedAndAverageColumns)
{
	script_highest();
	const auto shopkeeper = build("shopkeeper");

	expect_scores(*shopkeeper, { 18, 0, 21, 19, 13, 12, 12 });
}

// end of file: MimicSpiderShopkeeperAbilityTest.cpp
