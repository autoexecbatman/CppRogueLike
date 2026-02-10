#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "src/Actor/Actor.h"
#include "src/Actor/Pickable.h"
#include "src/Systems/ItemEnhancements/ItemEnhancements.h"
#include "src/Factories/ItemCreator.h"

using json = nlohmann::json;

// ============================================================================
// ITEM SERIALIZATION TESTS
// Ensures items save/load with all fields including enhancements
// ============================================================================

class ItemSerializationTest : public ::testing::Test {
protected:
    std::unique_ptr<Item> create_test_item() {
        auto item = std::make_unique<Item>(
            Vector2D{5, 10},
            ActorData{'/', "Test Sword", 1}
        );
        item->value = 100;
        item->base_value = 80;
        item->itemId = ItemId::LONG_SWORD;
        item->itemClass = ItemClass::SWORD;
        item->pickable = std::make_unique<Weapon>(false, HandRequirement::ONE_HANDED, WeaponSize::MEDIUM);
        return item;
    }
};

TEST_F(ItemSerializationTest, BasicFields_SaveLoad_RoundTrip) {
    auto original = create_test_item();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->value, 100);
    EXPECT_EQ(loaded->base_value, 80);
    EXPECT_EQ(loaded->itemId, ItemId::LONG_SWORD);
    EXPECT_EQ(loaded->itemClass, ItemClass::SWORD);
    EXPECT_EQ(loaded->actorData.name, "Test Sword");
}

TEST_F(ItemSerializationTest, Enhancement_Preserved) {
    auto original = create_test_item();

    // Set enhancement
    original->enhancement.prefix = PrefixType::SHARP;
    original->enhancement.suffix = SuffixType::OF_THE_BEAR;
    original->enhancement.damage_bonus = 2;
    original->enhancement.to_hit_bonus = 1;
    original->enhancement.strength_bonus = 2;
    original->enhancement.is_magical = true;
    original->enhancement.enhancement_level = 3;
    original->enhancement.value_modifier = 150;

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->enhancement.prefix, PrefixType::SHARP);
    EXPECT_EQ(loaded->enhancement.suffix, SuffixType::OF_THE_BEAR);
    EXPECT_EQ(loaded->enhancement.damage_bonus, 2);
    EXPECT_EQ(loaded->enhancement.to_hit_bonus, 1);
    EXPECT_EQ(loaded->enhancement.strength_bonus, 2);
    EXPECT_TRUE(loaded->enhancement.is_magical);
    EXPECT_EQ(loaded->enhancement.enhancement_level, 3);
    EXPECT_EQ(loaded->enhancement.value_modifier, 150);
}

TEST_F(ItemSerializationTest, Enhancement_AllResistances_Preserved) {
    auto original = create_test_item();

    original->enhancement.fire_resistance = 25;
    original->enhancement.cold_resistance = 30;
    original->enhancement.lightning_resistance = 15;
    original->enhancement.poison_resistance = 50;

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->enhancement.fire_resistance, 25);
    EXPECT_EQ(loaded->enhancement.cold_resistance, 30);
    EXPECT_EQ(loaded->enhancement.lightning_resistance, 15);
    EXPECT_EQ(loaded->enhancement.poison_resistance, 50);
}

TEST_F(ItemSerializationTest, Enhancement_CursedAndBlessed_Preserved) {
    auto original = create_test_item();

    original->enhancement.is_cursed = true;
    original->enhancement.is_blessed = false;

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    EXPECT_TRUE(loaded->enhancement.is_cursed);
    EXPECT_FALSE(loaded->enhancement.is_blessed);
}

TEST_F(ItemSerializationTest, Pickable_Preserved) {
    auto original = create_test_item();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    ASSERT_NE(loaded->pickable, nullptr) << "Pickable component not loaded";
}

TEST_F(ItemSerializationTest, NoEnhancement_DefaultValues) {
    auto original = create_test_item();
    // Don't set any enhancement - should use defaults

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->enhancement.prefix, PrefixType::NONE);
    EXPECT_EQ(loaded->enhancement.suffix, SuffixType::NONE);
    EXPECT_EQ(loaded->enhancement.damage_bonus, 0);
    EXPECT_FALSE(loaded->enhancement.is_cursed);
    EXPECT_FALSE(loaded->enhancement.is_blessed);
    EXPECT_FALSE(loaded->enhancement.is_magical);
}

TEST_F(ItemSerializationTest, AllItemClasses_SaveLoad) {
    std::vector<ItemClass> classes = {
        ItemClass::DAGGER,
        ItemClass::SWORD,
        ItemClass::GREAT_SWORD,
        ItemClass::POTION,
        ItemClass::ARMOR,
        ItemClass::SHIELD,
        ItemClass::SCROLL,
        ItemClass::FOOD
    };

    for (ItemClass itemClass : classes) {
        auto item = std::make_unique<Item>(Vector2D{0, 0}, ActorData{'?', "test", 1});
        item->itemClass = itemClass;
        item->value = 50;
        item->base_value = 40;

        json j;
        item->save(j);

        auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
        loaded->load(j);

        EXPECT_EQ(loaded->itemClass, itemClass) << "ItemClass mismatch for " << static_cast<int>(itemClass);
    }
}

// ----------------------------------------------------------------------------
// State Serialization Tests (Regression: auto-equip on load bug)
// ----------------------------------------------------------------------------

TEST_F(ItemSerializationTest, States_SavedAndLoaded) {
    auto original = create_test_item();
    original->add_state(ActorState::IS_EQUIPPED);

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    EXPECT_TRUE(loaded->has_state(ActorState::IS_EQUIPPED));
}

TEST_F(ItemSerializationTest, States_NotEquipped_StaysUnequipped) {
    auto original = create_test_item();
    // Don't add IS_EQUIPPED state

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    EXPECT_FALSE(loaded->has_state(ActorState::IS_EQUIPPED));
}

TEST_F(ItemSerializationTest, States_LoadClearsPreviousStates) {
    // Regression test: loading should clear existing states, not accumulate
    auto original = create_test_item();
    // Original has NO equipped state

    json j;
    original->save(j);

    // Create item and give it a state BEFORE loading
    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->add_state(ActorState::IS_EQUIPPED);  // Pre-existing state
    ASSERT_TRUE(loaded->has_state(ActorState::IS_EQUIPPED));

    // Load should REPLACE states, not ADD to them
    loaded->load(j);

    // After loading, the pre-existing state should be gone
    EXPECT_FALSE(loaded->has_state(ActorState::IS_EQUIPPED))
        << "Loading should clear previous states, not accumulate";
}

TEST_F(ItemSerializationTest, States_MultipleStates_AllPreserved) {
    auto original = create_test_item();
    original->add_state(ActorState::IS_EQUIPPED);
    original->add_state(ActorState::BLOCKS);

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{' ', "temp", 0});
    loaded->load(j);

    EXPECT_TRUE(loaded->has_state(ActorState::IS_EQUIPPED));
    EXPECT_TRUE(loaded->has_state(ActorState::BLOCKS));
}
