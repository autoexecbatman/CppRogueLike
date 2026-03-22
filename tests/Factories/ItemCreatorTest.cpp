#include "src/Factories/ItemCreator.h"
#include "src/Utils/Vector2D.h"
#include "src/Actor/Item.h"
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
}

TEST_F(ItemCreatorTest, CreateScrollLightning)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("scroll_lightning", pos, mock.content_registry);

	EXPECT_EQ(item->actorData.name, "scroll of lightning bolt");
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->get_value(), 150);
}

TEST_F(ItemCreatorTest, CreateRandomPotion)
{
	Vector2D pos(0, 0);
	auto ctx = mock.to_game_context();
	auto item = ItemCreator::create_random_of_category("potion", pos, ctx, 1);

	ASSERT_NE(item, nullptr);
	EXPECT_TRUE(item->behavior.has_value());
}

TEST_F(ItemCreatorTest, CreateLeatherArmor)
{
	Vector2D pos(0, 0);
	auto item = ItemCreator::create("leather_armor", pos, mock.content_registry);

	EXPECT_EQ(item->actorData.name, "leather armor");
	EXPECT_TRUE(item->behavior.has_value());
	EXPECT_EQ(item->get_value(), 5);
}
