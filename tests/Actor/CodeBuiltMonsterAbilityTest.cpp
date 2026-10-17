// file: CodeBuiltMonsterAbilityTest.cpp
// The five monsters built in code - the mimic, the three spiders and the shopkeeper -
// roll all six ability scores on the Dungeon Master Guide's Tables 6 to 11, read by
// their size and Intelligence rating, as a monster from data does. Until issues/0198
// moves them to data, each size and rating is a constant in its constructor:
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
//     build\bin\Debug\test_exe.exe --gtest_filter=CodeBuiltMonsterAbilityTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/Monsters.h"
#include "src/RandomDice.h"
#include "src/ShopkeeperFactory.h"
#include "src/Spider.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// More rolls than any of these constructors makes, so none falls back to a random one.
constexpr int SCRIPTED_ROLLS = 200;

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

class CodeBuiltMonsterAbilityTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
	}

	// Every die from here on at its highest.
	void script_highest()
	{
		for (int roll = 0; roll < SCRIPTED_ROLLS; ++roll)
		{
			mock.dice.set_next_roll(RandomDice::HIGHEST);
		}
	}

	MockGameContext mock{};
	GameContext ctx{};
};

// Table 6 large 20, 7 large 19, 8 large 21; Tables 9 to 11 Semi- 6, 6, 6.
TEST_F(CodeBuiltMonsterAbilityTest, AMimicReadsTheLargeAndSemiIntelligentColumns)
{
	script_highest();
	const Mimic mimic{ Vector2D{ 0, 0 }, ctx };

	expect_scores(mimic, { 20, 0, 19, 21, 6, 6, 6 });
}

// A 13 on Table 6 for a large creature is 18/51.
TEST_F(CodeBuiltMonsterAbilityTest, AMimicCarriesExceptionalStrength)
{
	for (const int die : { 6, 6, 1 })
	{
		mock.dice.set_next_roll(die);
	}
	script_highest();
	const Mimic mimic{ Vector2D{ 0, 0 }, ctx };

	EXPECT_EQ(mimic.get_strength(), 18);
	EXPECT_EQ(mimic.get_exceptional_strength(), 51);
}

// The hairy spider: tiny 15, 25, 15; Low 9, 9, 9.
TEST_F(CodeBuiltMonsterAbilityTest, ASmallSpiderReadsTheTinyAndLowColumns)
{
	script_highest();
	const SmallSpider spider{ Vector2D{ 0, 0 }, ctx };

	expect_scores(spider, { 15, 0, 25, 15, 9, 9, 9 });
}

// The huge spider: man-sized 18, 21, 19; Animal 4, 5, 3.
TEST_F(CodeBuiltMonsterAbilityTest, AGiantSpiderReadsTheManSizedAndAnimalColumns)
{
	script_highest();
	const GiantSpider spider{ Vector2D{ 0, 0 }, ctx };

	expect_scores(spider, { 18, 0, 21, 19, 4, 5, 3 });
}

// The giant spider: large 20, 19, 21; Low 9, 9, 9.
TEST_F(CodeBuiltMonsterAbilityTest, AWebSpinnerReadsTheLargeAndLowColumns)
{
	script_highest();
	const WebSpinner spider{ Vector2D{ 0, 0 }, ctx };

	expect_scores(spider, { 20, 0, 19, 21, 9, 9, 9 });
}

// A human: man-sized 18, 21, 19; Average 13, 12, 12.
TEST_F(CodeBuiltMonsterAbilityTest, AShopkeeperReadsTheManSizedAndAverageColumns)
{
	script_highest();
	const std::unique_ptr<Creature> shopkeeper = ShopkeeperFactory::create_shopkeeper(Vector2D{ 0, 0 }, 1, ctx);

	expect_scores(*shopkeeper, { 18, 0, 21, 19, 13, 12, 12 });
}

// end of file: CodeBuiltMonsterAbilityTest.cpp
