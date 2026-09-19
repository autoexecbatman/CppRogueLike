#include "src/ItemCreator.h"
#include "src/Vector2D.h"
#include "src/Item.h"
#include "tests/mocks/MockGameContext.h"
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

class ItemCreatorTest : public ::testing::Test
{
protected:
	MockGameContext mock;
	GameContext ctx{ mock.to_game_context() };
};

TEST_F(ItemCreatorTest, CreateHealthPotion)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("health_potion", pos, ctx);

	EXPECT_EQ(item->actorData.name, "health potion");
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->get_value(), 50);
	EXPECT_EQ(item->itemClass, ItemClass::POTION);
}

TEST_F(ItemCreatorTest, CreateScrollLightning)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("scroll_lightning", pos, ctx);

	EXPECT_EQ(item->actorData.name, "scroll of lightning bolt");
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->get_value(), 150);
	EXPECT_EQ(item->itemClass, ItemClass::SCROLL);
	ASSERT_TRUE(std::holds_alternative<TargetedScroll>(*item->behavior));
	const auto& scroll = std::get<TargetedScroll>(*item->behavior);
	EXPECT_EQ(scroll.targetMode, TargetMode::AUTO_NEAREST);
	EXPECT_EQ(scroll.damage, 20);
	EXPECT_EQ(scroll.range, 5);
}

TEST_F(ItemCreatorTest, CreateRandomPotion)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create_random_of_category("potion", pos, ctx, 1);

	ASSERT_NE(item, nullptr);
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->itemClass, ItemClass::POTION);
}

TEST_F(ItemCreatorTest, CreateLeatherArmor)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("leather_armor", pos, ctx);

	EXPECT_EQ(item->actorData.name, "leather armor");
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->get_value(), 5);
	EXPECT_EQ(item->itemClass, ItemClass::ARMOR);
}

TEST_F(ItemCreatorTest, CreateIdentifyScroll)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("identify_scroll", pos, ctx);

	EXPECT_EQ(item->actorData.name, "identify scroll");
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->itemClass, ItemClass::SCROLL);
	ASSERT_TRUE(std::holds_alternative<IdentifyScroll>(*item->behavior));
}

// The two resistance potions grant the buff DamageResolver reads as a strength
// in rings: a drunk potion is one ring's worth for its duration. Pinned here
// because the data once carried no effect at all, which no code path could
// notice: a potion that does nothing looks like a potion.
TEST_F(ItemCreatorTest, FireResistancePotionIsOneRingsWorth)
{
	auto item = ItemCreator::create("potion_of_fire_resistance", Vector2D{ 0, 0 }, ctx);
	ASSERT_TRUE(item);
	ASSERT_TRUE(std::holds_alternative<Consumable>(*item->behavior));
	const auto& potion = std::get<Consumable>(*item->behavior);

	EXPECT_EQ(potion.effect, ConsumableEffect::ADD_BUFF);
	EXPECT_EQ(potion.buffType, BuffType::FIRE_RESISTANCE);
	EXPECT_EQ(potion.amount, 1) << "the buff value is the strength in rings";
	EXPECT_EQ(potion.duration, 50);
	EXPECT_FALSE(potion.isSetEffect);
}

TEST_F(ItemCreatorTest, ColdResistancePotionIsOneRingsWorth)
{
	auto item = ItemCreator::create("potion_of_cold_resistance", Vector2D{ 0, 0 }, ctx);
	ASSERT_TRUE(item);
	ASSERT_TRUE(std::holds_alternative<Consumable>(*item->behavior));
	const auto& potion = std::get<Consumable>(*item->behavior);

	EXPECT_EQ(potion.effect, ConsumableEffect::ADD_BUFF);
	EXPECT_EQ(potion.buffType, BuffType::COLD_RESISTANCE);
	EXPECT_EQ(potion.amount, 1);
	EXPECT_EQ(potion.duration, 50);
}

// A gem is authored as worth 100 and reaches the player as a Gold behaviour, which is
// the only thing pickup and use read. Two items carry pickableType gold_coin - the
// rolled pile, which ItemFactory builds through create_with_gold_amount, and the gem,
// which comes through the ordinary create() path like every other item.
TEST_F(ItemCreatorTest, AGemIsWorthWhatItIsAuthoredToBeWorth)
{
	const int authored = mock.itemRegistry.get_params("gem").value;
	ASSERT_GT(authored, 0) << "the data must give a gem a value for this to mean anything";

	auto item = ItemCreator::create("gem", Vector2D{ 0, 0 }, ctx);
	ASSERT_TRUE(item);
	ASSERT_TRUE(std::holds_alternative<Gold>(*item->behavior));

	EXPECT_EQ(std::get<Gold>(*item->behavior).amount, authored)
		<< "a gem picked up gives the player this many gold, and nothing else reads its value";
}

// baseWeight decides how often an item is drawn from its category; weight decides what
// it costs to carry. They were one field, so a health potion at draw-weight 50 weighed
// fifty times a suit of full plate at 1.
//
// The two are seeded identically in the shipped data, so an assertion against the real
// file cannot tell which one is read. This writes a file where they differ.
TEST_F(ItemCreatorTest, AnItemWeighsItsWeightRatherThanItsSpawnRate)
{
	const std::filesystem::path apart =
		std::filesystem::temp_directory_path() / "items_weight_apart.json";

	std::ifstream source(std::filesystem::path{ "data/content/items.json" });
	ASSERT_TRUE(source.is_open());
	nlohmann::json root = nlohmann::json::parse(source);
	ASSERT_TRUE(root.contains("health_potion"));
	root["health_potion"]["baseWeight"] = 50;
	root["health_potion"]["weight"] = 7;

	std::ofstream out(apart);
	out << root.dump(2);
	out.close();

	mock.itemRegistry.load(apart.string());
	auto item = ItemCreator::create("health_potion", Vector2D{ 0, 0 }, ctx);
	ASSERT_TRUE(item);
	const int carried = item->enhancement.weight;

	std::filesystem::remove(apart);

	EXPECT_EQ(carried, 7)
		<< "an item must carry its weight, not its draw weight of 50";
}

// The other half of the same separation: how often an item is drawn must not move when
// its carry weight changes. Both fields are seeded identically in the shipped data, so
// only a file where they disagree can tell which one the draw reads.
//
// health_potion sorts first and invisibility_potion second, so with draw weights of 1
// and 100 a roll of 50 walks past the first and lands on the second. Were the draw to
// read carry weight - 100 and 1 - the same roll would stop on the first.
TEST_F(ItemCreatorTest, CarryWeightDoesNotChangeHowOftenAnItemIsDrawn)
{
	const std::filesystem::path apart =
		std::filesystem::temp_directory_path() / "items_draw_apart.json";

	std::ifstream source(std::filesystem::path{ "data/content/items.json" });
	ASSERT_TRUE(source.is_open());
	nlohmann::json root = nlohmann::json::parse(source);
	for (auto& [key, record] : root.items())
	{
		if (record["category"] == "potion")
		{
			// Every other potion drops out of the draw entirely.
			record["baseWeight"] = 0;
		}
	}
	// Level scaling would multiply the draw weights; hold it flat so the roll is exact.
	root["health_potion"]["baseWeight"] = 1;
	root["health_potion"]["weight"] = 100;
	root["health_potion"]["levelScaling"] = 0.0;
	root["invisibility_potion"]["baseWeight"] = 100;
	root["invisibility_potion"]["weight"] = 1;
	root["invisibility_potion"]["levelScaling"] = 0.0;

	std::ofstream out(apart);
	out << root.dump(2);
	out.close();

	mock.itemRegistry.load(apart.string());
	ctx.dice->set_next_roll(50);
	auto drawn = ItemCreator::create_random_of_category("potion", Vector2D{ 0, 0 }, ctx, 1);

	std::filesystem::remove(apart);

	ASSERT_TRUE(drawn);
	EXPECT_EQ(drawn->itemKey, "invisibility_potion")
		<< "the draw must weigh by baseWeight; reading carry weight lands on health_potion";
}
