// file: CorpseRecordTest.cpp
// A monster's corpse is data on its record: what it is called, how much eating it feeds,
// whether it can be eaten, and the line printed when it is eaten or refused. The record gives them to the creature, the
// creature gives them to the corpse it leaves, and the corpse gives them to whoever eats
// it. Every value here is a fixture's, so the tests pin the path rather than the data.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CorpseRecordTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/CreatureSize.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/HungerSystem.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/MessageSystem.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/RandomDice.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr std::string_view FIXTURE_NAME = "rubble of a fixture";
constexpr int FIXTURE_NUTRITION = 37;
constexpr std::string_view FIXTURE_TASTE = "It tastes of the fixture.";
constexpr std::string_view FIXTURE_REFUSAL = "The fixture is not for eating.";
constexpr Vector2D WHERE_IT_FELL{ 7, 9 };

// The weight every record held before its corpse was weighed, 28 of them at once.
constexpr int PLACEHOLDER_CORPSE_WEIGHT = 100;

// A size's corpse weight in pounds, as ruled 2026-10-18 for every record that had none.
int ruled_corpse_weight(CreatureSize size)
{
	switch (size)
	{
	case CreatureSize::TINY:
	{
		return 5;
	}
	case CreatureSize::SMALL:
	{
		return 50;
	}
	case CreatureSize::MEDIUM:
	{
		return 150;
	}
	case CreatureSize::LARGE:
	{
		return 400;
	}
	case CreatureSize::HUGE_SIZE:
	{
		return 1500;
	}
	case CreatureSize::GARGANTUAN:
	{
		return 3000;
	}
	}
	std::unreachable();
}

// How far an authored weight may sit from its size's: the dragon, gargantuan at 600
// against 3000, is the widest, so five is the least factor that admits every record.
constexpr int SIZE_BAND_FACTOR = 5;
} // namespace

class CorpseRecordTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->set_creature_class(CreatureClass::FIGHTER);
	}

	void TearDown() override { std::filesystem::remove(edited); }

	// A creature whose corpse is the fixture's.
	std::unique_ptr<Creature> fixture_creature() const
	{
		auto creature = std::make_unique<Creature>(
			WHERE_IT_FELL,
			ActorData{ TileRef{}, "fixture", ColorPairId::WHITE_BLACK });
		creature->experienceReward = std::make_unique<ExperienceReward>(1);
		creature->healthPool = std::make_unique<HealthPool>(8);
		creature->armorClass = std::make_unique<ArmorClass>(6);
		creature->set_corpse(
			std::string{ FIXTURE_NAME },
			FIXTURE_NUTRITION,
			std::string{ FIXTURE_TASTE },
			CorpseEdibility::EDIBLE);
		return creature;
	}

	// The corpse on the floor, found by the fixture's name.
	Item* fixture_corpse() const
	{
		const auto isTheCorpse = [](const std::unique_ptr<Item>& item)
		{
			return item && item->actorData.name == FIXTURE_NAME;
		};
		const auto found =
			std::ranges::find_if(ctx.floorInventory->items, isTheCorpse);
		if (found == ctx.floorInventory->items.end())
		{
			return nullptr;
		}
		return found->get();
	}

	// monsters.json with one field taken off the goblin, written where load can read it.
	void write_without(std::string_view field)
	{
		std::ifstream source(Paths::resolve(Paths::MONSTERS));
		nlohmann::json root = nlohmann::json::parse(source);
		ASSERT_TRUE(root.at("goblin").contains(std::string{ field }))
			<< "the goblin never carried " << field;
		root.at("goblin").erase(std::string{ field });
		std::ofstream out(edited);
		out << root.dump();
	}

	// Whether any stored message holds the given text, its coloured runs joined.
	bool said(std::string_view wanted) const
	{
		for (
			size_t index = 0;
			index < ctx.messageSystem->get_stored_message_count();
			++index)
		{
			std::string whole;
			for (
				const LogMessage& run :
				ctx.messageSystem->get_attack_message_at(index))
			{
				whole += run.text;
			}
			if (whole.find(wanted) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(
		Vector2D{ 1, 1 }) };
	std::filesystem::path edited{ std::filesystem::temp_directory_path() /
		"monsters_corpse.json" };
};

TEST_F(CorpseRecordTest, ARecordWithoutCorpseNutritionRefusesToLoad)
{
	write_without("corpse_nutrition");
	MonsterRegistry monsters{};

	EXPECT_ANY_THROW(monsters.load(edited.string()));
}

TEST_F(CorpseRecordTest, ARecordWithoutACorpseTasteRefusesToLoad)
{
	write_without("corpse_taste");
	MonsterRegistry monsters{};

	EXPECT_ANY_THROW(monsters.load(edited.string()));
}

TEST_F(CorpseRecordTest, ARecordWithoutCorpseEdibleRefusesToLoad)
{
	write_without("corpse_edible");
	MonsterRegistry monsters{};

	EXPECT_ANY_THROW(monsters.load(edited.string()));
}

// The creator copies the record's corpse onto the creature it builds.
TEST_F(CorpseRecordTest, AMonsterIsBuiltCarryingItsRecordsCorpse)
{
	MonsterParams params = mock.monsterRegistry.get_params("goblin");
	params.corpseName = std::string{ FIXTURE_NAME };
	params.corpseNutrition = FIXTURE_NUTRITION;
	params.corpseTaste = std::string{ FIXTURE_TASTE };

	const auto built =
		MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(built->get_corpse_name(), FIXTURE_NAME);
	EXPECT_EQ(built->get_corpse_nutrition(), FIXTURE_NUTRITION);
	EXPECT_EQ(built->get_corpse_taste(), FIXTURE_TASTE);
}

// An inedible record builds a creature whose corpse is inedible.
TEST_F(CorpseRecordTest, AMonsterIsBuiltWithItsRecordsCorpseEdibility)
{
	MonsterParams params = mock.monsterRegistry.get_params("goblin");
	params.corpseEdibility = CorpseEdibility::INEDIBLE;

	const auto built =
		MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(built->get_corpse_edibility(), CorpseEdibility::INEDIBLE);
}

// An inedible creature leaves an inedible corpse.
TEST_F(CorpseRecordTest, AnInedibleCreatureLeavesAnInedibleCorpse)
{
	auto creature = fixture_creature();
	creature->set_corpse(
		std::string{ FIXTURE_NAME },
		FIXTURE_NUTRITION,
		std::string{ FIXTURE_REFUSAL },
		CorpseEdibility::INEDIBLE);
	creature->die(ctx);

	const Item* corpse = fixture_corpse();
	ASSERT_NE(corpse, nullptr);
	ASSERT_TRUE(corpse->behavior.has_value());
	const CorpseFood* food = std::get_if<CorpseFood>(&*corpse->behavior);
	ASSERT_NE(food, nullptr);
	EXPECT_EQ(food->edibility, CorpseEdibility::INEDIBLE);
}

// Eating an inedible corpse is refused with its line: it stays in the pack, hunger does
// not move, and the use reports nothing done, so no time passes.
TEST_F(CorpseRecordTest, AnInedibleCorpseIsRefusedAndKept)
{
	auto corpse = std::make_unique<Item>(
		Vector2D{ 0, 0 },
		ActorData{
			TileRef{},
			std::string{ FIXTURE_NAME },
			ColorPairId::WHITE_BLACK,
		});
	corpse->behavior = CorpseFood{
		FIXTURE_NUTRITION,
		std::string{ FIXTURE_REFUSAL },
		CorpseEdibility::INEDIBLE,
	};
	Item& held = *corpse;
	ASSERT_TRUE(
		InventoryOperations::add_item(player->inventoryData, std::move(corpse))
			.has_value());
	ctx.hungerSystem->increase_hunger(ctx, 500);
	const int before = ctx.hungerSystem->get_hunger_value();

	const bool used = use_item(*held.behavior, held, *player, ctx);

	EXPECT_FALSE(used) << "a refused meal reported itself as done";
	EXPECT_EQ(ctx.hungerSystem->get_hunger_value(), before)
		<< "a refused meal fed the eater";
	EXPECT_EQ(InventoryOperations::get_item_count(player->inventoryData), 1u)
		<< "a refused corpse left the pack";
	EXPECT_TRUE(said(FIXTURE_REFUSAL)) << "the refusal line was not printed";
	EXPECT_FALSE(said("You eat the")) << "the refusal said it was eaten";
}

// The body left behind is named as the record names it, not "dead" and the creature's name.
TEST_F(CorpseRecordTest, ADeadCreatureLeavesItsRecordsCorpse)
{
	fixture_creature()->die(ctx);

	const Item* corpse = fixture_corpse();
	ASSERT_NE(corpse, nullptr)
		<< "no corpse named \"" << FIXTURE_NAME << "\" was left";
	ASSERT_TRUE(corpse->behavior.has_value());
	const CorpseFood* food = std::get_if<CorpseFood>(&*corpse->behavior);
	ASSERT_NE(food, nullptr) << "the corpse is not food";
	EXPECT_EQ(food->nutritionValue, FIXTURE_NUTRITION);
	EXPECT_EQ(food->taste, FIXTURE_TASTE);
}

// Eating feeds the corpse's nutrition, give or take ten, and prints its taste. A scripted 0
// takes the variation out.
TEST_F(CorpseRecordTest, EatingACorpseFeedsItsNutritionAndSaysItsTaste)
{
	fixture_creature()->die(ctx);
	Item* corpse = fixture_corpse();
	ASSERT_NE(corpse, nullptr);
	ASSERT_TRUE(corpse->behavior.has_value());
	// Hungry enough that the meal cannot hit the floor of zero.
	ctx.hungerSystem->increase_hunger(ctx, 500);
	const int before = ctx.hungerSystem->get_hunger_value();
	mock.dice.set_next_roll(0);

	use_item(*corpse->behavior, *corpse, *player, ctx);

	EXPECT_EQ(before - ctx.hungerSystem->get_hunger_value(), FIXTURE_NUTRITION);
	EXPECT_TRUE(said(FIXTURE_TASTE))
		<< "eating did not print the corpse's taste";
}

// A corpse lying on the floor across a save keeps what eating it does.
TEST_F(CorpseRecordTest, ACorpseKeepsItsNutritionAndTasteAcrossASave)
{
	Item original{
		Vector2D{ 0, 0 },
		ActorData{
			TileRef{},
			std::string{ FIXTURE_NAME },
			ColorPairId::WHITE_BLACK,
		},
	};
	original.behavior = CorpseFood{
		FIXTURE_NUTRITION,
		std::string{ FIXTURE_TASTE },
		CorpseEdibility::INEDIBLE,
	};
	nlohmann::json record;
	original.save(record);

	Item loaded{
		Vector2D{ 0, 0 },
		ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK },
	};
	loaded.load(record);

	ASSERT_TRUE(loaded.behavior.has_value());
	const CorpseFood* food = std::get_if<CorpseFood>(&*loaded.behavior);
	ASSERT_NE(food, nullptr);
	EXPECT_EQ(food->nutritionValue, FIXTURE_NUTRITION);
	EXPECT_EQ(food->taste, FIXTURE_TASTE);
	EXPECT_EQ(food->edibility, CorpseEdibility::INEDIBLE)
		<< "an inedible corpse became edible across a save";
}

// A living creature across a save keeps the corpse it will leave.
TEST_F(CorpseRecordTest, ACreatureKeepsItsCorpseAcrossASave)
{
	const auto original = fixture_creature();
	original->set_corpse(
		std::string{ FIXTURE_NAME },
		FIXTURE_NUTRITION,
		std::string{ FIXTURE_TASTE },
		CorpseEdibility::INEDIBLE);
	nlohmann::json record;
	original->save(record);

	Creature loaded{
		Vector2D{ 0, 0 },
		ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK },
	};
	loaded.load(record);

	EXPECT_EQ(loaded.get_corpse_name(), FIXTURE_NAME);
	EXPECT_EQ(loaded.get_corpse_nutrition(), FIXTURE_NUTRITION);
	EXPECT_EQ(loaded.get_corpse_taste(), FIXTURE_TASTE);
	EXPECT_EQ(loaded.get_corpse_edibility(), CorpseEdibility::INEDIBLE)
		<< "an inedible creature became edible across a save";
}

// Every record says what its corpse feeds and how it tastes.
TEST_F(CorpseRecordTest, EveryRecordCarriesACorpseNutritionAndTaste)
{
	for (const std::string& key : mock.monsterRegistry.get_all_keys())
	{
		const MonsterParams& params = mock.monsterRegistry.get_params(key);
		EXPECT_GE(params.corpseNutrition, 0) << key;
		EXPECT_FALSE(params.corpseTaste.empty())
			<< key << " has no eating line";
		EXPECT_EQ(params.corpseTaste.find("PLACEHOLDER"), std::string::npos)
			<< key << " still holds a placeholder";
	}
}

// 100 was copied onto 28 records in place of a weight, a rat's among them.
TEST_F(CorpseRecordTest, NoRecordKeepsThePlaceholderCorpseWeight)
{
	for (const std::string& key : mock.monsterRegistry.get_all_keys())
	{
		EXPECT_NE(
			mock.monsterRegistry.get_params(key).corpseWeight,
			PLACEHOLDER_CORPSE_WEIGHT)
			<< key;
	}
}

// A corpse weighs within a factor of five of its size's weight, so a weight copied from a
// record of another size fails. A shadow and a wraith dissipate, and weigh almost nothing.
TEST_F(CorpseRecordTest, EveryCorpseWeighsWithinItsSize)
{
	const std::set<std::string> dissipated{ "shadow", "wraith" };
	for (const std::string& key : mock.monsterRegistry.get_all_keys())
	{
		const MonsterParams& params = mock.monsterRegistry.get_params(key);
		if (dissipated.contains(key))
		{
			EXPECT_EQ(params.corpseWeight, 1) << key;
			continue;
		}
		const int sizeWeight = ruled_corpse_weight(params.size);
		EXPECT_GE(params.corpseWeight * SIZE_BAND_FACTOR, sizeWeight)
			<< key << " is light for its size";
		EXPECT_LE(params.corpseWeight, sizeWeight * SIZE_BAND_FACTOR)
			<< key << " is heavy for its size";
	}
}

// The owner's ruling, 2026-10-18: these four corpses cannot be eaten, and every other can.
TEST_F(CorpseRecordTest, OnlyTheOddCorpsesAreInedible)
{
	const std::set<std::string> inedible{
		"golem_stone",
		"shadow",
		"skeleton",
		"wraith",
	};
	for (const std::string& key : mock.monsterRegistry.get_all_keys())
	{
		const CorpseEdibility expected = inedible.contains(key) ?
			CorpseEdibility::INEDIBLE :
			CorpseEdibility::EDIBLE;
		EXPECT_EQ(
			mock.monsterRegistry.get_params(key).corpseEdibility,
			expected)
			<< key;
	}
}

// end of file: CorpseRecordTest.cpp
