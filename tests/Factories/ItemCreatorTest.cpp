#include <gtest/gtest.h>
#include "tests/mocks/MockGameContext.h"
#include "src/Factories/ItemCreator.h"
#include "src/Utils/Vector2D.h"

class ItemCreatorTest : public ::testing::Test
{
protected:
    MockGameContext mock;
};

TEST_F(ItemCreatorTest, CreateHealthPotion)
{
    Vector2D pos(0, 0);
    auto item = ItemCreator::create(ItemId::HEALTH_POTION, pos);

    EXPECT_EQ(item->actorData.ch, '!');
    EXPECT_EQ(item->actorData.name, "health potion");
    EXPECT_NE(item->pickable, nullptr);
    EXPECT_EQ(item->value, 50);
}

TEST_F(ItemCreatorTest, CreateScrollLightning)
{
    Vector2D pos(0, 0);
    auto item = ItemCreator::create(ItemId::SCROLL_LIGHTNING, pos);

    EXPECT_EQ(item->actorData.ch, '?');
    EXPECT_EQ(item->actorData.name, "scroll of lightning bolt");
    EXPECT_NE(item->pickable, nullptr);
    EXPECT_EQ(item->value, 150);
}

TEST_F(ItemCreatorTest, CreateRandomPotion)
{
    Vector2D pos(0, 0);
    auto ctx = mock.to_game_context();
    auto item = ItemCreator::create_random_potion(pos, ctx, 1);

    EXPECT_EQ(item->actorData.ch, '!');
    EXPECT_NE(item->pickable, nullptr);
    // Verify it's one of the valid random potions
    EXPECT_TRUE(
        item->actorData.name == "health potion" ||
        item->actorData.name == "mana potion" ||
        item->actorData.name == "invisibility potion"
    );
}

TEST_F(ItemCreatorTest, CalculateEnhancementChance)
{
    EXPECT_EQ(ItemCreator::calculate_enhancement_chance(1), 5);
    EXPECT_EQ(ItemCreator::calculate_enhancement_chance(2), 8);
    EXPECT_EQ(ItemCreator::calculate_enhancement_chance(3), 11);
    EXPECT_EQ(ItemCreator::calculate_enhancement_chance(10), 32);
    EXPECT_EQ(ItemCreator::calculate_enhancement_chance(11), 35);
}

TEST_F(ItemCreatorTest, DetermineEnhancementLevel)
{
    auto ctx = mock.to_game_context();

    int level = ItemCreator::determine_enhancement_level(ctx, 1);
    EXPECT_GE(level, 0);
    EXPECT_LE(level, 3);

    level = ItemCreator::determine_enhancement_level(ctx, 5);
    EXPECT_GE(level, 0);
    EXPECT_LE(level, 3);
}

TEST_F(ItemCreatorTest, CreateEnhancedDagger)
{
    Vector2D pos(0, 0);
    auto item = ItemCreator::create_enhanced_weapon(ItemId::DAGGER, pos, 1);

    EXPECT_EQ(item->actorData.color, WHITE_GREEN_PAIR);
    EXPECT_NE(item, nullptr);
}

TEST_F(ItemCreatorTest, CreateLeatherArmor)
{
    Vector2D pos(0, 0);
    auto item = ItemCreator::create(ItemId::LEATHER_ARMOR, pos);

    EXPECT_EQ(item->actorData.ch, '[');
    EXPECT_EQ(item->actorData.name, "leather armor");
    EXPECT_NE(item->pickable, nullptr);
    EXPECT_EQ(item->value, 5);
}
