// file: MonsterHitDiceCountsTest.cpp
// A monster record's "hit_dice": the die, a bonus added once to the total, and the counts
// the monster may have, each with the experience it is worth. A Monstrous Manual entry
// such as the killer mimic's "Hit Dice: 9-10" with "XP Value: 9 HD: 2,000" (p1700) is two
// counts; every other record is one.
//
// What it pins: the loader refuses hit dice the book cannot write; a one-count monster
// spends no die choosing its count, so its dice are what they were; a monster with several
// rolls the count, and its level, hit points and experience all follow that roll; and an
// encounter budget pays the highest count's experience.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterHitDiceCountsTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <string>
#include <vector>

#include "src/Creature.h"
#include "src/CreatureSize.h"
#include "src/DataManager.h"
#include "src/EncounterPlanner.h"
#include "src/ExperienceReward.h"
#include "src/LevelManager.h"
#include "src/MonsterCreator.h"
#include "src/MonsterHitDice.h"
#include "src/MonsterRegistry.h"
#include "src/MonsterStrength.h"
#include "src/Paths.h"
#include "tests/mocks/MockGameContext.h"

class MonsterHitDiceCountsTest : public ::testing::Test
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

	void TearDown() override { std::filesystem::remove(edited); }

	void force_rolls(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			mock.dice.set_next_roll(roll);
		}
	}

	// Dexterity's 3d6, a Constitution of 17 (18 for a man-sized creature, +4 a die), then
	// Intelligence, Wisdom and Charisma's 3d6: every die a man-sized monster with a printed
	// Strength rolls before its hit dice.
	void force_ability_scores()
	{
		force_rolls({ 3, 3, 3, 6, 6, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3 });
	}

	// A man-sized goblin record with a printed Strength and the caller's hit dice.
	MonsterParams params_with(MonsterHitDice hitDice)
	{
		MonsterParams params = mock.monsterRegistry.get_params("goblin");
		params.equipment.clear();
		params.size = CreatureSize::MEDIUM;
		params.strength = MonsterStrength{ 10, 0 };
		params.hitDice = hitDice;
		return params;
	}

	// monsters.json with the orc's "hit_dice" replaced, written where a load can read it.
	std::string with_orc_hit_dice(const nlohmann::json& hitDice)
	{
		std::ifstream source(Paths::resolve(Paths::MONSTERS));
		nlohmann::json root = nlohmann::json::parse(source);
		root.at("orc")["hit_dice"] = hitDice;
		std::ofstream out(edited);
		out << root.dump();
		return edited.string();
	}

	static nlohmann::json hit_dice(
		int sides,
		int bonus,
		std::initializer_list<HitDiceCount> counts)
	{
		nlohmann::json entries = nlohmann::json::array();
		for (const HitDiceCount& entry : counts)
		{
			entries.push_back({ { "count", entry.count }, { "xp", entry.xp } });
		}
		return {
			{ "sides", sides },
			{ "bonus", bonus },
			{ "counts", entries },
		};
	}

	MonsterRegistry monsters{};
	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	LevelManager levelManager{};
	std::filesystem::path edited{ std::filesystem::temp_directory_path() /
		"monsters_hit_dice.json" };
};

TEST_F(MonsterHitDiceCountsTest, NoCountIsRefused)
{
	EXPECT_THROW(
		monsters.load(with_orc_hit_dice(hit_dice(8, 0, {}))),
		std::runtime_error);
}

TEST_F(MonsterHitDiceCountsTest, AGapBetweenCountsIsRefused)
{
	EXPECT_THROW(
		monsters.load(
			with_orc_hit_dice(hit_dice(8, 0, { { 9, 2000 }, { 11, 3000 } }))),
		std::runtime_error);
}

TEST_F(MonsterHitDiceCountsTest, DescendingCountsAreRefused)
{
	EXPECT_THROW(
		monsters.load(
			with_orc_hit_dice(hit_dice(8, 0, { { 10, 3000 }, { 9, 2000 } }))),
		std::runtime_error);
}

// The count is the creature's level, and the book's below-one dice are count 1 with a
// negative bonus.
TEST_F(MonsterHitDiceCountsTest, ACountOfZeroIsRefused)
{
	EXPECT_THROW(
		monsters.load(with_orc_hit_dice(hit_dice(8, 0, { { 0, 10 } }))),
		std::runtime_error);
}

TEST_F(MonsterHitDiceCountsTest, NegativeExperienceIsRefused)
{
	EXPECT_THROW(
		monsters.load(with_orc_hit_dice(hit_dice(8, 0, { { 1, -1 } }))),
		std::runtime_error);
}

TEST_F(MonsterHitDiceCountsTest, ADieOfNoSidesIsRefused)
{
	EXPECT_THROW(
		monsters.load(with_orc_hit_dice(hit_dice(0, 0, { { 1, 10 } }))),
		std::runtime_error);
}

TEST_F(MonsterHitDiceCountsTest, ARecordWithoutHitDiceIsRefused)
{
	std::ifstream source(Paths::resolve(Paths::MONSTERS));
	nlohmann::json root = nlohmann::json::parse(source);
	root.at("orc").erase("hit_dice");
	std::ofstream out(edited);
	out << root.dump();
	out.close();

	EXPECT_THROW(monsters.load(edited.string()), nlohmann::json::out_of_range);
}

// The edges beside each refusal load: one count of 1, a one-sided die, no experience, and
// the mimic's two counts.
TEST_F(MonsterHitDiceCountsTest, TheEdgesOfEachRuleLoad)
{
	EXPECT_NO_THROW(
		monsters.load(with_orc_hit_dice(hit_dice(1, 99, { { 1, 0 } }))));
	EXPECT_NO_THROW(
		monsters.load(with_orc_hit_dice(hit_dice(8, -1, { { 1, 65 } }))));
	EXPECT_NO_THROW(monsters.load(
		with_orc_hit_dice(hit_dice(8, 0, { { 9, 2000 }, { 10, 3000 } }))));
}

// The file's numbers survive the load: the dragon's 15d8 at 12,000 (Red Dragon p1240 and its
// age table, p1242).
TEST_F(MonsterHitDiceCountsTest, ARecordsHitDiceAreRead)
{
	const MonsterHitDice& dragon = monsters.get_params("dragon").hitDice;
	EXPECT_EQ(dragon.sides, 8);
	EXPECT_EQ(dragon.bonus, 0);
	ASSERT_EQ(dragon.counts.size(), 1u);
	EXPECT_EQ(dragon.counts.front().count, 15);
	EXPECT_EQ(dragon.counts.front().xp, 12000);
}

// One count: no die chooses it, so the hit dice are the next dice after the ability scores.
// Two d8 at 5, each +4 for Constitution 18, are 18.
TEST_F(MonsterHitDiceCountsTest, AOneCountMonsterSpendsNoDieChoosingItsCount)
{
	const MonsterParams params =
		params_with(MonsterHitDice{ 8, 0, { { 2, 40 } } });
	force_ability_scores();
	force_rolls({ 5, 5 });

	const auto monster =
		MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_creature_level(), 2);
	EXPECT_EQ(monster->get_max_hp(), 18)
		<< "a die was spent before the hit dice";
	EXPECT_EQ(monster->experienceReward->get_xp(), 40);
}

// The mimic's two counts, rolling 10: level 10, ten d8 at 1 each +4 for 50, and 3,000.
TEST_F(MonsterHitDiceCountsTest, ARolledCountSetsTheLevelHitPointsAndExperience)
{
	const MonsterParams params =
		params_with(MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } });
	force_ability_scores();
	force_rolls({ 10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });

	const auto monster =
		MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_creature_level(), 10);
	EXPECT_EQ(monster->get_max_hp(), 50);
	EXPECT_EQ(monster->experienceReward->get_xp(), 3000);
}

// Rolling 9: nine dice for 45, and 2,000.
TEST_F(MonsterHitDiceCountsTest, TheLowerCountIsWorthItsOwnExperience)
{
	const MonsterParams params =
		params_with(MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } });
	force_ability_scores();
	force_rolls({ 9, 1, 1, 1, 1, 1, 1, 1, 1, 1 });

	const auto monster =
		MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_creature_level(), 9);
	EXPECT_EQ(monster->get_max_hp(), 45);
	EXPECT_EQ(monster->experienceReward->get_xp(), 2000);
}

// A budget pays for the worst roll, so a monster of 9 or 10 hit dice costs 3,000.
TEST_F(
	MonsterHitDiceCountsTest,
	AnEncounterBudgetPaysTheHighestCountsExperience)
{
	MonsterParams ranged = monsters.get_params("orc");
	ranged.name = "Ranged Count Beast";
	ranged.hitDice = MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } };
	ranged.levelMinimum = 1;
	ranged.levelMaximum = 0;
	const std::string key = monsters.add_custom(ranged);

	const std::vector<MonsterCandidate> candidates =
		encounter_candidates(monsters, 1);

	const auto isTheBeast = [&key](const MonsterCandidate& candidate)
	{
		return candidate.key == key;
	};
	const auto found = std::ranges::find_if(candidates, isTheBeast);
	ASSERT_NE(found, candidates.end())
		<< "the two-count monster was not a candidate";
	EXPECT_EQ(found->xpCost, 3000);
}

// The editor widens a range at its bottom: the new counts are worth the old lowest's.
TEST(MonsterHitDiceRangeTest, LoweringTheLowestCountAddsCountsWorthTheOldLowest)
{
	const MonsterHitDice widened = with_lowest_count(
		MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } },
		7);

	ASSERT_EQ(widened.counts.size(), 4u);
	EXPECT_EQ(widened.counts.at(0).count, 7);
	EXPECT_EQ(widened.counts.at(0).xp, 2000);
	EXPECT_EQ(widened.counts.at(1).count, 8);
	EXPECT_EQ(widened.counts.at(2).count, 9);
	EXPECT_EQ(widened.counts.at(3).xp, 3000);
	EXPECT_TRUE(is_valid_monster_hit_dice(widened));
}

TEST(
	MonsterHitDiceRangeTest,
	RaisingTheLowestCountDropsCountsAndStopsAtTheHighest)
{
	const MonsterHitDice narrowed = with_lowest_count(
		MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } },
		12);

	ASSERT_EQ(narrowed.counts.size(), 1u);
	EXPECT_EQ(narrowed.counts.front().count, 10);
	EXPECT_EQ(narrowed.counts.front().xp, 3000);
}

TEST(MonsterHitDiceRangeTest, TheLowestCountStopsAtOne)
{
	const MonsterHitDice floored =
		with_lowest_count(MonsterHitDice{ 8, 0, { { 2, 40 } } }, -3);

	ASSERT_EQ(floored.counts.size(), 2u);
	EXPECT_EQ(floored.counts.front().count, 1);
	EXPECT_EQ(floored.counts.front().xp, 40);
}

TEST(
	MonsterHitDiceRangeTest,
	RaisingTheHighestCountAddsCountsWorthTheOldHighest)
{
	const MonsterHitDice widened =
		with_highest_count(MonsterHitDice{ 8, 0, { { 9, 2000 } } }, 11);

	ASSERT_EQ(widened.counts.size(), 3u);
	EXPECT_EQ(widened.counts.at(1).count, 10);
	EXPECT_EQ(widened.counts.at(2).count, 11);
	EXPECT_EQ(widened.counts.at(2).xp, 2000);
	EXPECT_TRUE(is_valid_monster_hit_dice(widened));
}

TEST(
	MonsterHitDiceRangeTest,
	LoweringTheHighestCountDropsCountsAndStopsAtTheLowest)
{
	const MonsterHitDice narrowed = with_highest_count(
		MonsterHitDice{ 8, 0, { { 9, 2000 }, { 10, 3000 } } },
		1);

	ASSERT_EQ(narrowed.counts.size(), 1u);
	EXPECT_EQ(narrowed.counts.front().count, 9);
	EXPECT_EQ(narrowed.counts.front().xp, 2000);
}

TEST(MonsterHitDiceRangeTest, TheHighestCountStopsAtNinetyNine)
{
	const MonsterHitDice capped =
		with_highest_count(MonsterHitDice{ 8, 0, { { 98, 10 } } }, 120);

	ASSERT_EQ(capped.counts.size(), 2u);
	EXPECT_EQ(capped.counts.back().count, 99);
}

// end of file: MonsterHitDiceCountsTest.cpp
