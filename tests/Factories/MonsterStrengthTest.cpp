// file: MonsterStrengthTest.cpp
// Where a monster's Strength comes from. Six Monstrous Manual entries print one, and
// those six carry it in monsters.json; every other monster rolls 3d6 on the DMG's
// Table 6 by its size when it is built (Ability Scores for Monsters, PDF page 4860).
//
// The six are the book's, one line each with the PDF page of the local archive and the
// words the figure is read from. Two print a bonus and no score, and take the score
// whose strength.json row gives that damage, by the driver's ruling of 2026-10-18: the
// bugbear's "+2 to damage" is 18, and the ogre's "by weapon +6" is 18/00, the Strength
// the DMG's gauntlets of ogre power grant.
//
// A Strength's damage adjustment lands on every natural attack (PDF page 4860), so the
// six deal the book's base dice with the adjustment left out of them; a record that
// kept the bonus in its dice would count it twice.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterStrengthTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

#include "src/Creature.h"
#include "src/CreatureSize.h"
#include "src/DataManager.h"
#include "src/LevelManager.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/MonsterStrength.h"
#include "src/Paths.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
struct BookStrength
{
	std::string_view key{};
	MonsterStrength strength{};
	std::string_view baseDamage{};
};

// Monstrous Manual, by PDF page in the local archive.
constexpr BookStrength BOOK_STRENGTHS[] = {
	// Baatezu p1103: "its incredible 18/00 Strength (+6 damage adjustment)". Its six
	// attacks print as "1-4x2/1-6x2/2-12/2-8" on p1102; the one this game makes is the
	// bite, 2-12.
	{ "pit_fiend", { 18, 100 }, "2d6" },
	// Vampire p1946: "a Strength score of 18/76, receiving a bonus of +2 to hit and +4 to
	// damage"; p1945 prints "Damage/Attack: 5-10", which is 1d6 and that +4.
	{ "vampire", { 18, 76 }, "1d6" },
	// Giant, Hill p1432: "oversized clubs which do 2-12 +7 points of damage (double the
	// damage of a man-sized club plus their strength bonus)". +7 is row 19.
	{ "hill_giant", { 19, 0 }, "2d6" },
	// Golem, Greater p1492: "Stone golems have a strength of 22"; "Damage/Attack: 3-24
	// (3d8)" on the same page.
	{ "golem_stone", { 22, 0 }, "3d8" },
	// Bugbear p1148: "Damage/Attack: 2-8 (2d4) or by weapon" and "Special Attacks:
	// Surprise, +2 to damage". +2 is row 18. It carries a morning star, also 2d4.
	{ "bugbear", { 18, 0 }, "2d4" },
	// Ogre p1751: "Damage/Attack: 1-10 (or by weapon +6)". +6 is 18/00. It carries a club,
	// 1d6.
	{ "ogre", { 18, 100 }, "1d6" },
};

bool prints_a_strength(std::string_view key)
{
	const auto isThisMonster = [key](const BookStrength& book)
	{
		return book.key == key;
	};
	return std::ranges::any_of(BOOK_STRENGTHS, isThisMonster);
}

// A score written as a fixed value rolls no dice.
DiceExpr fixed_score(int score)
{
	return DiceExpr{ 0, 0, score };
}
} // namespace

class MonsterStrengthTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		monsters.load(Paths::MONSTERS);
		dataManager.load_all_data(mock.messages);
		ctx = mock.to_game_context();
		ctx.dataManager = &dataManager;
		ctx.levelManager = &levelManager;
	}

	void TearDown() override
	{
		std::filesystem::remove(edited);
	}

	void force_rolls(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			mock.dice.set_next_roll(roll);
		}
	}

	// A goblin of the caller's size and Strength whose other rolled scores are fixed, so
	// the dice rolled are its Strength's 3d6 if it has none, then its Constitution's.
	MonsterParams params_with(CreatureSize size, std::optional<MonsterStrength> strength)
	{
		MonsterParams params = mock.monsterRegistry.get_params("goblin");
		params.equipment.clear();
		params.size = size;
		params.strength = strength;
		params.dexDice = fixed_score(10);
		params.intDice = fixed_score(10);
		params.wisDice = fixed_score(10);
		params.chaDice = fixed_score(10);
		return params;
	}

	// monsters.json with one record's "strength" changed by the caller, written where a
	// load can read it.
	std::string with_orc_strength(const nlohmann::json& strength)
	{
		std::ifstream source(Paths::resolve(Paths::MONSTERS));
		nlohmann::json root = nlohmann::json::parse(source);
		root.at("orc")["strength"] = strength;
		write(root);
		return edited.string();
	}

	void write(const nlohmann::json& root)
	{
		std::ofstream out(edited);
		out << root.dump();
	}

	MonsterRegistry monsters{};
	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	LevelManager levelManager{};
	std::filesystem::path edited{ std::filesystem::temp_directory_path() / "monsters_strength.json" };
};

TEST_F(MonsterStrengthTest, EachMonsterWhoseEntryPrintsAStrengthCarriesIt)
{
	for (const BookStrength& book : BOOK_STRENGTHS)
	{
		const std::optional<MonsterStrength>& held = monsters.get_params(std::string{ book.key }).strength;
		ASSERT_TRUE(held.has_value()) << book.key << " carries no Strength, so it would roll Table 6";
		EXPECT_EQ(held->score, book.strength.score) << book.key;
		EXPECT_EQ(held->exceptional, book.strength.exceptional) << book.key;
	}
}

TEST_F(MonsterStrengthTest, EveryOtherMonsterRollsTable6)
{
	for (const std::string& key : monsters.get_all_keys())
	{
		if (!prints_a_strength(key))
		{
			EXPECT_FALSE(monsters.get_params(key).strength.has_value()) << key << " carries a Strength its entry does not print";
		}
	}
}

TEST_F(MonsterStrengthTest, EachOfTheSixDealsTheBooksBaseDice)
{
	for (const BookStrength& book : BOOK_STRENGTHS)
	{
		EXPECT_EQ(monsters.get_params(std::string{ book.key }).damage.displayRoll, book.baseDamage)
			<< book.key << " deals other dice than its entry prints, or keeps its Strength bonus in them";
	}
}

// A large monster rolling 13 reads 18/51 from Table 6, and both halves land.
TEST_F(MonsterStrengthTest, AMonsterWithNoStrengthOfItsOwnRollsTable6BySize)
{
	const MonsterParams params = params_with(CreatureSize::LARGE, std::nullopt);
	force_rolls({ 4, 4, 5 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_strength(), 18);
	EXPECT_EQ(monster->get_exceptional_strength(), 51);
}

// The printed Strength is the monster's, and it rolls nothing for it: the first three
// dice are its Constitution's, 17 for a man-sized creature reading 18 on Table 8.
TEST_F(MonsterStrengthTest, AMonsterWithAStrengthOfItsOwnKeepsItAndRollsNoDieForIt)
{
	const MonsterParams params = params_with(CreatureSize::MEDIUM, MonsterStrength{ 18, 100 });
	force_rolls({ 6, 6, 5 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_strength(), 18);
	EXPECT_EQ(monster->get_exceptional_strength(), 100);
	EXPECT_EQ(monster->get_constitution(), 18) << "a die was rolled for a Strength the record already holds";
}

// The key is required: a record without it is unfinished rather than a Table 6 monster.
TEST_F(MonsterStrengthTest, ARecordWithoutAStrengthKeyIsRefused)
{
	std::ifstream source(Paths::resolve(Paths::MONSTERS));
	nlohmann::json root = nlohmann::json::parse(source);
	root.at("orc").erase("strength");
	write(root);

	EXPECT_THROW(monsters.load(edited.string()), nlohmann::json::out_of_range);
}

// A percentile rides only on an 18.
TEST_F(MonsterStrengthTest, APercentileBesideAScoreOtherThan18IsRefused)
{
	EXPECT_THROW(monsters.load(with_orc_strength({ { "score", 17 }, { "exceptional", 50 } })), std::runtime_error);
	EXPECT_THROW(monsters.load(with_orc_strength({ { "score", 19 }, { "exceptional", 1 } })), std::runtime_error);
}

TEST_F(MonsterStrengthTest, AScoreWithNoStrengthRowIsRefused)
{
	EXPECT_THROW(monsters.load(with_orc_strength({ { "score", 26 }, { "exceptional", 0 } })), std::runtime_error);
	EXPECT_THROW(monsters.load(with_orc_strength({ { "score", 0 }, { "exceptional", 0 } })), std::runtime_error);
}

TEST_F(MonsterStrengthTest, APercentileAbove100IsRefused)
{
	EXPECT_THROW(monsters.load(with_orc_strength({ { "score", 18 }, { "exceptional", 101 } })), std::runtime_error);
}

// The edges the refusals sit beside load.
TEST_F(MonsterStrengthTest, The18_00And25AndAPlainScoreLoad)
{
	EXPECT_NO_THROW(monsters.load(with_orc_strength({ { "score", 18 }, { "exceptional", 100 } })));
	EXPECT_NO_THROW(monsters.load(with_orc_strength({ { "score", 25 }, { "exceptional", 0 } })));
	EXPECT_NO_THROW(monsters.load(with_orc_strength({ { "score", 1 }, { "exceptional", 0 } })));
	EXPECT_NO_THROW(monsters.load(with_orc_strength(nullptr)));
}

// end of file: MonsterStrengthTest.cpp
