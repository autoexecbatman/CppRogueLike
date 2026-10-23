// file: MonsterAbilityScoresTest.cpp
// A monster built from monsters.json reads all six ability scores from the DMG's tables
// (Ability Scores for Monsters, PDF pages 4857-4860): Strength, Dexterity and Constitution
// by its size from Tables 6 to 8, and Intelligence, Wisdom and Charisma by its Intelligence
// rating from Tables 9 to 11. No Monstrous Manual entry for a monster in the game prints a
// Dexterity, Intelligence, Wisdom or Charisma score, so nothing in the data overrides them,
// and no record carries ability dice.
//
// The dice are read in the book's order - Strength, Dexterity, Constitution, Intelligence,
// Wisdom, Charisma - with no Strength die when the record prints one. Expected values are
// the tables' printed cells, columns named beside each.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterAbilityScoresTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <initializer_list>
#include <optional>
#include <string_view>

#include "src/Creature.h"
#include "src/CreatureSize.h"
#include "src/DataManager.h"
#include "src/IntelligenceRating.h"
#include "src/LevelManager.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/MonsterStrength.h"
#include "src/Paths.h"
#include "tests/mocks/MockGameContext.h"

class MonsterAbilityScoresTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		dataManager.load_all_data(mock.messages);
		ctx = mock.to_game_context();
		ctx.dataManager = &dataManager;
		ctx.levelManager = &levelManager;
	}

	void force_rolls(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			mock.dice.set_next_roll(roll);
		}
	}

	// A goblin record of the caller's size, rating and Strength, carrying nothing, so the
	// only dice rolled before its hit dice are its ability scores'.
	MonsterParams params_with(
		CreatureSize size,
		IntelligenceRating rating,
		std::optional<MonsterStrength> strength)
	{
		MonsterParams params = mock.monsterRegistry.get_params("goblin");
		params.equipment.clear();
		params.size = size;
		params.intelligence = rating;
		params.strength = strength;
		return params;
	}

	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	LevelManager levelManager{};
};

// A printed Strength rolls no die, so the first three are Dexterity's. Tiny reads Tables 7
// and 8's first column; Low is Tables 9 to 11's third.
TEST_F(
	MonsterAbilityScoresTest,
	ATinyLowMonsterReadsEachTableAndRollsNoStrengthDie)
{
	const MonsterParams params = params_with(
		CreatureSize::TINY,
		IntelligenceRating::LOW,
		MonsterStrength{ 10, 0 });
	force_rolls({ 1, 1, 1, 6, 6, 6, 1, 1, 1, 6, 6, 6, 6, 6, 6 });

	const auto monster =
		MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_strength(), 10) << "the printed Strength";
	EXPECT_EQ(monster->get_dexterity(), 13)
		<< "Table 7, a 3 in the tiny column";
	EXPECT_EQ(monster->get_constitution(), 15)
		<< "Table 8, an 18 in the tiny column";
	EXPECT_EQ(monster->get_intelligence(), 3)
		<< "Table 9, a 3 in the Low column";
	EXPECT_EQ(monster->get_wisdom(), 9) << "Table 10, an 18 in the Low column";
	EXPECT_EQ(monster->get_charisma(), 9)
		<< "Table 11, an 18 in the Low column";
}

// With no printed Strength the first three dice are Table 6's. A non-intelligent creature
// has Intelligence 0 and reads Wisdom and Charisma from the Animal column.
TEST_F(
	MonsterAbilityScoresTest,
	ALargeNonIntelligentMonsterReadsEachTableInTheBooksOrder)
{
	const MonsterParams params =
		params_with(CreatureSize::LARGE, IntelligenceRating::NON, std::nullopt);
	force_rolls({ 1, 1, 1, 6, 6, 6, 1, 1, 1, 3, 3, 3, 6, 6, 6, 6, 6, 6 });

	const auto monster =
		MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_strength(), 13)
		<< "Table 6, a 3 in the large column";
	EXPECT_EQ(monster->get_dexterity(), 19)
		<< "Table 7, an 18 in the large column";
	EXPECT_EQ(monster->get_constitution(), 9)
		<< "Table 8, a 3 in the large column";
	EXPECT_EQ(monster->get_intelligence(), 0) << "Non- (0), whatever it rolls";
	EXPECT_EQ(monster->get_wisdom(), 5)
		<< "Table 10, an 18 in the Animal column";
	EXPECT_EQ(monster->get_charisma(), 3)
		<< "Table 11, an 18 in the Animal column";
}

// Issue 0199's own success line: the scores come from the tables, so no record writes dice
// for them. Read as raw JSON, since the parser would ignore a key it no longer reads.
TEST_F(MonsterAbilityScoresTest, NoRecordCarriesAbilityDice)
{
	std::ifstream source(Paths::resolve(Paths::MONSTERS));
	const nlohmann::json root = nlohmann::json::parse(source);

	for (const auto& [key, record] : root.items())
	{
		for (
			const std::string_view diceKey :
			{ "str", "dex", "int", "wis", "cha" })
		{
			EXPECT_FALSE(record.contains(diceKey))
				<< key << " still carries \"" << diceKey << "\" dice";
		}
	}
}

// end of file: MonsterAbilityScoresTest.cpp
