#include <gtest/gtest.h>
#include <vector>
#include <string>

// Include the target header files
#include "src/Factories/ItemCreator.h"
#include "src/Utils/Vector2D.h"

class ItemCreatorTest : public ::testing::Test {
protected:
    ItemCreator creator;
};

TEST_F(ItemCreatorTest, CreateHealthPotion) {
    Vector2D pos(0, 0);
    auto item = creator.create_health_potion(pos);

    EXPECT_EQ(item->actorData.ch, '!');
    EXPECT_EQ(item->actorData.name, "health potion");
    EXPECT_NE(item->pickable, nullptr);
    EXPECT_EQ(item->value, 50);
}

TEST_F(ItemCreatorTest, CreateScrollLightning) {
    Vector2D pos(0, 0);
    auto item = creator.create_scroll_lightning(pos);

    EXPECT_EQ(item->actorData.ch, '?');
    EXPECT_EQ(item->actorData.name, "scroll of lightning bolt");
    EXPECT_NE(item->pickable, nullptr);
    EXPECT_EQ(item->value, 150);
}

TEST_F(ItemCreatorTest, CreateRandomPotion) {
    Vector2D pos(0, 0);
    auto item = creator.create_random_potion(pos, 1);

    EXPECT_EQ(item->actorData.ch, '!');
    EXPECT_EQ(item->actorData.name, "health potion");
    EXPECT_NE(item->pickable, nullptr);
    EXPECT_EQ(item->value, 50);
}

TEST_F(ItemCreatorTest, CalculateEnhancementChance) {
    int chance = creator.calculate_enhancement_chance(1);
    EXPECT_EQ(chance, 5);

    chance = creator.calculate_enhancement_chance(2);
    EXPECT_EQ(chance, 8);

    chance = creator.calculate_enhancement_chance(3);
    EXPECT_EQ(chance, 11);

    chance = creator.calculate_enhancement_chance(10);
    EXPECT_EQ(chance, 32);  // 5 + (10-1)*3 = 32

    chance = creator.calculate_enhancement_chance(11);
    EXPECT_EQ(chance, 35);  // 5 + (11-1)*3 = 35 (capped)
}

TEST_F(ItemCreatorTest, DetermineEnhancementLevel) {
    int level = creator.determine_enhancement_level(1);
    EXPECT_GE(level, 0);
    EXPECT_LE(level, 3);

    level = creator.determine_enhancement_level(5);
    EXPECT_GE(level, 0);
    EXPECT_LE(level, 3);
}

TEST_F(ItemCreatorTest, CreateEnhancedDagger) {
    Vector2D pos(0, 0);
    auto item = creator.create_enhanced_dagger(pos, 1);

    EXPECT_EQ(item->actorData.color, WHITE_GREEN_PAIR);
    EXPECT_NE(item, nullptr);
    // Note: value calculation depends on game state and enhancement system
}

TEST_F(ItemCreatorTest, CreateLeatherArmor) {
    Vector2D pos(0, 0);
    auto item = creator.create_leather_armor(pos);

    EXPECT_EQ(item->actorData.ch, '[');
    EXPECT_EQ(item->actorData.name, "leather armor");
    EXPECT_NE(item->pickable, nullptr);
    EXPECT_EQ(item->value, 5);
}
