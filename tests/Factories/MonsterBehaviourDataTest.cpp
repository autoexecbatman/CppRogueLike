// file: MonsterBehaviourDataTest.cpp
// What a monster record says about behaviour, and what is built from it: the Ai named
// by its save name ("monster", "ranged_monster", "mimic", "spider", "giant_spider",
// "web_spinner", "shopkeeper"), whether it walks webs, its attitude, and whether the
// player may swap places with it. A Confused Ai wraps another and is never authored, so
// a record naming it is refused.
//
// The shopkeeper has no Monstrous Manual entry. Its record is a human with 1d1+99 hit
// points - level 1, and 100 before its Constitution - peaceful and not displaceable, and
// its Ai opens its shop once it is built. It is spawned only through its own gate, never
// as an ordinary entry in the spawn table.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterBehaviourDataTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "src/Ai.h"
#include "src/Attitude.h"
#include "src/Creature.h"
#include "src/DataManager.h"
#include "src/EncounterPlanner.h"
#include "src/LevelManager.h"
#include "src/MonsterCreator.h"
#include "src/MonsterFactory.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"
#include "src/RandomDice.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// More rolls than building a shopkeeper and stocking its shop makes.
constexpr int SCRIPTED_ROLLS = 400;

// A record key and the Ai it names.
struct NamedAi
{
	std::string key{};
	AiType type{ AiType::MONSTER };
};
} // namespace

class MonsterBehaviourDataTest : public ::testing::Test
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

	std::unique_ptr<Creature> build(const std::string& key)
	{
		return MonsterCreator::create_from_params(Vector2D{ 0, 0 }, monsters.get_params(key), ctx);
	}

	MonsterRegistry monsters{};
	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	LevelManager levelManager{};
	std::filesystem::path edited{ std::filesystem::temp_directory_path() / "monsters_behaviour.json" };
};

// Each monster is built with the Ai its record names.
TEST_F(MonsterBehaviourDataTest, EachMonsterIsBuiltWithTheAiItsRecordNames)
{
	const std::vector<NamedAi> named = {
		{ "goblin", AiType::MONSTER },
		{ "archer", AiType::RANGED_MONSTER },
		{ "spider_weaver", AiType::WEB_SPINNER },
		{ "shopkeeper", AiType::SHOPKEEPER },
	};
	for (const NamedAi& record : named)
	{
		const auto monster = build(record.key);
		ASSERT_TRUE(monster->ai) << record.key;
		EXPECT_EQ(monster->ai->get_ai_type(), record.type) << record.key;
	}
}

TEST_F(MonsterBehaviourDataTest, AConfusedAiInARecordIsRefused)
{
	std::ifstream source(Paths::resolve(Paths::MONSTERS));
	nlohmann::json root = nlohmann::json::parse(source);
	root.at("orc")["ai"] = "confused_monster";
	std::ofstream out(edited);
	out << root.dump();
	out.close();

	EXPECT_THROW(monsters.load(edited.string()), std::runtime_error);
}

// The three behaviour fields as the records give them: spiders cross webs, the
// shopkeeper is peaceful and holds its ground, and an ordinary monster is none of these.
TEST_F(MonsterBehaviourDataTest, TheRecordsCarryWebsAttitudeAndDisplacement)
{
	EXPECT_TRUE(monsters.get_params("spider_small").walksWebs);
	EXPECT_FALSE(monsters.get_params("goblin").walksWebs);

	EXPECT_EQ(monsters.get_params("shopkeeper").attitude, Attitude::PEACEFUL);
	EXPECT_EQ(monsters.get_params("goblin").attitude, Attitude::HOSTILE);

	EXPECT_FALSE(monsters.get_params("shopkeeper").displaceable);
	EXPECT_TRUE(monsters.get_params("goblin").displaceable);
}

// Built, the shopkeeper is peaceful, cannot be swapped with, and has a shop. Every die at
// its highest gives Constitution 19 (Table 8, man-sized), worth 5 a die as a warrior
// (Player's Handbook Table 3), so its one 1d1+99 die is 1 + 99 + 5.
TEST_F(MonsterBehaviourDataTest, AShopkeeperIsBuiltPeacefulImmovableAndWithAShop)
{
	for (int roll = 0; roll < SCRIPTED_ROLLS; ++roll)
	{
		mock.dice.set_next_roll(RandomDice::HIGHEST);
	}

	const auto shopkeeper = build("shopkeeper");

	EXPECT_EQ(shopkeeper->get_attitude(), Attitude::PEACEFUL);
	EXPECT_FALSE(shopkeeper->is_displaceable());
	EXPECT_TRUE(shopkeeper->shop) << "the shopkeeper was built with no shop";
	EXPECT_EQ(shopkeeper->get_creature_level(), 1);
	EXPECT_EQ(shopkeeper->get_constitution(), 19);
	EXPECT_EQ(shopkeeper->get_max_hp(), 105);
}

// The shopkeeper reaches the spawn table once, through its own gated entry, and never as
// an ordinary monster beside it.
TEST_F(MonsterBehaviourDataTest, TheShopkeeperIsNeverAnOrdinarySpawn)
{
	const std::vector<MonsterPercentage> distribution = MonsterFactory::get_current_distribution(1, monsters);

	const auto sameLetter = [](unsigned char left, unsigned char right)
	{
		return std::tolower(left) == std::tolower(right);
	};
	const auto isAShopkeeper = [&sameLetter](const MonsterPercentage& entry)
	{
		return std::ranges::equal(entry.name, std::string_view{ "shopkeeper" }, sameLetter);
	};
	EXPECT_EQ(std::ranges::count_if(distribution, isAShopkeeper), 1);
}

// The shopkeeper is worth no experience, which is what keeps the encounter planner from
// filling a room with shopkeepers at any depth.
TEST_F(MonsterBehaviourDataTest, TheShopkeeperIsNeverPlannedIntoAnEncounter)
{
	const auto isAShopkeeper = [](const MonsterCandidate& candidate)
	{
		return candidate.key == "shopkeeper";
	};
	for (int dungeonLevel = 1; dungeonLevel <= 10; ++dungeonLevel)
	{
		EXPECT_FALSE(std::ranges::any_of(encounter_candidates(monsters, dungeonLevel), isAShopkeeper)) << "level " << dungeonLevel;
	}
}

// end of file: MonsterBehaviourDataTest.cpp
