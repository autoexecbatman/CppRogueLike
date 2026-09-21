#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "src/Item.h"
#include "src/Pickable.h"
#include "src/ItemEnhancements.h"
#include "src/ItemCreator.h"

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
            ActorData{TileRef{}, "Test Sword", 1}
        );
        item->set_value(80);
        item->itemKey = "long_sword";
        item->itemClass = ItemClass::SWORD;
        item->behavior = Weapon{ false, HandRequirement::ONE_HANDED, WeaponSize::MEDIUM };
        return item;
    }
};

TEST_F(ItemSerializationTest, BasicFields_SaveLoad_RoundTrip) {
    auto original = create_test_item();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->get_value(), 80);
    EXPECT_EQ(loaded->itemKey, "long_sword");
    EXPECT_EQ(loaded->itemClass, ItemClass::SWORD);
    EXPECT_EQ(loaded->actorData.name, "Test Sword");
}

TEST_F(ItemSerializationTest, Enhancement_Preserved) {
    auto original = create_test_item();

    // Set enhancement
    original->enhancement.prefix = PrefixType::SHARP;
    original->enhancement.suffix = SuffixType::OF_THE_BEAR;
    original->enhancement.damageBonus = 2;
    original->enhancement.toHitBonus = 1;
    original->enhancement.strengthBonus = 2;
    original->enhancement.isMagical = true;
    original->enhancement.enhancementLevel = 3;
    original->enhancement.valueModifier = 150;

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->enhancement.prefix, PrefixType::SHARP);
    EXPECT_EQ(loaded->enhancement.suffix, SuffixType::OF_THE_BEAR);
    EXPECT_EQ(loaded->enhancement.damageBonus, 2);
    EXPECT_EQ(loaded->enhancement.toHitBonus, 1);
    EXPECT_EQ(loaded->enhancement.strengthBonus, 2);
    EXPECT_TRUE(loaded->enhancement.isMagical);
    EXPECT_EQ(loaded->enhancement.enhancementLevel, 3);
    EXPECT_EQ(loaded->enhancement.valueModifier, 150);
}

TEST_F(ItemSerializationTest, Enhancement_AllResistances_Preserved) {
    auto original = create_test_item();

    original->enhancement.fireResistance = 25;
    original->enhancement.coldResistance = 30;
    original->enhancement.lightningResistance = 15;
    original->enhancement.poisonResistance = 50;

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->enhancement.fireResistance, 25);
    EXPECT_EQ(loaded->enhancement.coldResistance, 30);
    EXPECT_EQ(loaded->enhancement.lightningResistance, 15);
    EXPECT_EQ(loaded->enhancement.poisonResistance, 50);
}

TEST_F(ItemSerializationTest, Enhancement_CursedAndBlessed_Preserved) {
    auto original = create_test_item();

    original->enhancement.blessing = BlessingStatus::CURSED;

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->enhancement.blessing, BlessingStatus::CURSED);
}

TEST_F(ItemSerializationTest, Pickable_Preserved) {
    auto original = create_test_item();

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    ASSERT_TRUE(loaded->behavior.has_value()) << "Pickable component not loaded";
}

TEST_F(ItemSerializationTest, NoEnhancement_DefaultValues) {
    auto original = create_test_item();
    // Don't set any enhancement - should use defaults

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_EQ(loaded->enhancement.prefix, PrefixType::NONE);
    EXPECT_EQ(loaded->enhancement.suffix, SuffixType::NONE);
    EXPECT_EQ(loaded->enhancement.damageBonus, 0);
    EXPECT_EQ(loaded->enhancement.blessing, BlessingStatus::UNCURSED);
    EXPECT_FALSE(loaded->enhancement.isMagical);
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
        auto item = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "test", 1});
        item->itemClass = itemClass;
        item->set_value(40);

        json j;
        item->save(j);

        auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
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

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_TRUE(loaded->has_state(ActorState::IS_EQUIPPED));
}

TEST_F(ItemSerializationTest, States_NotEquipped_StaysUnequipped) {
    auto original = create_test_item();
    // Don't add IS_EQUIPPED state

    json j;
    original->save(j);

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
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
    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
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

    auto loaded = std::make_unique<Item>(Vector2D{0, 0}, ActorData{TileRef{}, "temp", 0});
    loaded->load(j);

    EXPECT_TRUE(loaded->has_state(ActorState::IS_EQUIPPED));
    EXPECT_TRUE(loaded->has_state(ActorState::BLOCKS));
}

// A two-handed weapon must still need two hands after a save and a load. The fixture
// above builds a one-handed sword, which is the default, so it could never see the
// requirement go missing - only a weapon that differs from the default can.
TEST_F(ItemSerializationTest, HandRequirement_Preserved)
{
	auto original = create_test_item();
	original->behavior = Weapon{ false, HandRequirement::TWO_HANDED, WeaponSize::LARGE, 0 };

	json j;
	original->save(j);

	auto loaded = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	loaded->load(j);

	ASSERT_TRUE(loaded->behavior.has_value());
	const Weapon* weapon = std::get_if<Weapon>(&*loaded->behavior);
	ASSERT_NE(weapon, nullptr) << "a saved weapon came back as something else";
	EXPECT_EQ(weapon->handRequirement, HandRequirement::TWO_HANDED)
		<< "a two-handed weapon came back one-handed";
	EXPECT_EQ(weapon->weaponSize, WeaponSize::LARGE) << "the weapon's size was lost";
	EXPECT_FALSE(weapon->ranged) << "a melee weapon came back ranged";
}

// A save record missing a weapon field is a broken record, not a one-handed weapon.
TEST_F(ItemSerializationTest, AWeaponRecordMissingAFieldIsRefused)
{
	auto original = create_test_item();
	original->behavior = Weapon{ false, HandRequirement::TWO_HANDED, WeaponSize::LARGE, 0 };

	json j;
	original->save(j);
	j["pickable"].erase("handRequirement");

	auto loaded = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	EXPECT_THROW(loaded->load(j), nlohmann::json::exception)
		<< "a weapon with no hand requirement loaded quietly as one-handed";
}

// A scroll's animation was written as "scrollAnimation" and read as "animation", so it
// came back as whatever the default is. Any value but the default proves it survives.
TEST_F(ItemSerializationTest, ScrollAnimation_Preserved)
{
	auto original = create_test_item();
	TargetedScroll scroll;
	scroll.scrollAnimation = ScrollAnimation::LIGHTNING;
	scroll.targetMode = TargetMode::AUTO_NEAREST;
	original->behavior = scroll;

	json j;
	original->save(j);

	auto loaded = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
	loaded->load(j);

	ASSERT_TRUE(loaded->behavior.has_value());
	const TargetedScroll* readBack = std::get_if<TargetedScroll>(&*loaded->behavior);
	ASSERT_NE(readBack, nullptr) << "a saved scroll came back as something else";
	EXPECT_EQ(readBack->scrollAnimation, ScrollAnimation::LIGHTNING)
		<< "the scroll's animation was lost in the save";
}
