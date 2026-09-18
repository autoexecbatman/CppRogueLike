#include "src/ItemCreator.h"
#include "src/Vector2D.h"
#include "src/Item.h"
#include "tests/mocks/MockGameContext.h"
#include <gtest/gtest.h>

class ItemCreatorTest : public ::testing::Test
{
protected:
	MockGameContext mock;

	void SetUp() override
	{
		ItemCreator::load("data/content/items.json");
	}
};

TEST_F(ItemCreatorTest, CreateHealthPotion)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("health_potion", pos, mock.content_registry);

	EXPECT_EQ(item->actorData.name, "health potion");
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->get_value(), 50);
	EXPECT_EQ(item->itemClass, ItemClass::POTION);
}

TEST_F(ItemCreatorTest, CreateScrollLightning)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("scroll_lightning", pos, mock.content_registry);

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
	auto ctx = mock.to_game_context();
	auto item = ItemCreator::create_random_of_category("potion", pos, ctx, 1);

	ASSERT_NE(item, nullptr);
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->itemClass, ItemClass::POTION);
}

TEST_F(ItemCreatorTest, CreateLeatherArmor)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("leather_armor", pos, mock.content_registry);

	EXPECT_EQ(item->actorData.name, "leather armor");
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->get_value(), 5);
	EXPECT_EQ(item->itemClass, ItemClass::ARMOR);
}

TEST_F(ItemCreatorTest, CreateIdentifyScroll)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("identify_scroll", pos, mock.content_registry);

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
	auto item = ItemCreator::create("potion_of_fire_resistance", Vector2D{ 0, 0 }, mock.content_registry);
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
	auto item = ItemCreator::create("potion_of_cold_resistance", Vector2D{ 0, 0 }, mock.content_registry);
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
	const int authored = ItemCreator::get_params("gem").value;
	ASSERT_GT(authored, 0) << "the data must give a gem a value for this to mean anything";

	auto item = ItemCreator::create("gem", Vector2D{ 0, 0 }, mock.content_registry);
	ASSERT_TRUE(item);
	ASSERT_TRUE(std::holds_alternative<Gold>(*item->behavior));

	EXPECT_EQ(std::get<Gold>(*item->behavior).amount, authored)
		<< "a gem picked up gives the player this many gold, and nothing else reads its value";
}
