#include <gtest/gtest.h>
#include "src/Combat/WeaponDamageRegistry.h"
#include "src/Combat/DamageInfo.h"
#include "src/Items/ItemClassification.h"

// ============================================================================
// THE WIZARD'S WEAPON REGISTRY TESTS
// Validates damage lookup system - core combat mechanic
// ============================================================================

class WeaponDamageRegistryTest : public ::testing::Test {
protected:
    // No setup needed - WeaponDamageRegistry is static
};

// Basic Lookups
TEST_F(WeaponDamageRegistryTest, GetDamageInfo_Dagger) {
    DamageInfo info = WeaponDamageRegistry::get_damage_info(ItemClass::DAGGER);

    EXPECT_EQ(info.minDamage, 1);
    EXPECT_EQ(info.maxDamage, 4);
    EXPECT_EQ(info.displayRoll, "1d4");
}

TEST_F(WeaponDamageRegistryTest, GetDamageInfo_LongSword) {
    DamageInfo info = WeaponDamageRegistry::get_damage_info(ItemClass::LONG_SWORD);

    EXPECT_EQ(info.minDamage, 1);
    EXPECT_EQ(info.maxDamage, 8);
    EXPECT_EQ(info.displayRoll, "1d8");
}

TEST_F(WeaponDamageRegistryTest, GetDamageInfo_GreatSword) {
    DamageInfo info = WeaponDamageRegistry::get_damage_info(ItemClass::GREAT_SWORD);

    EXPECT_EQ(info.minDamage, 1);
    EXPECT_EQ(info.maxDamage, 10);
}

TEST_F(WeaponDamageRegistryTest, GetDamageInfo_BattleAxe) {
    DamageInfo info = WeaponDamageRegistry::get_damage_info(ItemClass::BATTLE_AXE);

    EXPECT_EQ(info.minDamage, 1);
    EXPECT_EQ(info.maxDamage, 8);
}

// Registration Status
TEST_F(WeaponDamageRegistryTest, IsRegistered_ValidWeapons) {
    EXPECT_TRUE(WeaponDamageRegistry::is_registered(ItemClass::DAGGER));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered(ItemClass::SHORT_SWORD));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered(ItemClass::LONG_SWORD));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered(ItemClass::GREAT_SWORD));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered(ItemClass::BATTLE_AXE));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered(ItemClass::WAR_HAMMER));
}

TEST_F(WeaponDamageRegistryTest, IsRegistered_NonWeapons) {
    // These are valid items but not weapons
    EXPECT_FALSE(WeaponDamageRegistry::is_registered(ItemClass::HEALTH_POTION));
    EXPECT_FALSE(WeaponDamageRegistry::is_registered(ItemClass::LEATHER_ARMOR));
    EXPECT_FALSE(WeaponDamageRegistry::is_registered(ItemClass::SCROLL_LIGHTNING));
}

// Unarmed Damage
TEST_F(WeaponDamageRegistryTest, GetUnarmedDamage) {
    DamageInfo unarmed = WeaponDamageRegistry::get_unarmed_damage_info();

    EXPECT_EQ(unarmed.minDamage, 1);
    EXPECT_EQ(unarmed.maxDamage, 2);
    EXPECT_EQ(unarmed.displayRoll, "1d2");
}

TEST_F(WeaponDamageRegistryTest, GetUnarmedDamageString) {
    std::string roll = WeaponDamageRegistry::get_unarmed_damage();
    EXPECT_EQ(roll, "1d2");
}

// Legacy Display Strings
TEST_F(WeaponDamageRegistryTest, GetDamageRoll_DisplayStrings) {
    EXPECT_EQ(WeaponDamageRegistry::get_damage_roll(ItemClass::DAGGER), "1d4");
    EXPECT_EQ(WeaponDamageRegistry::get_damage_roll(ItemClass::LONG_SWORD), "1d8");
    EXPECT_EQ(WeaponDamageRegistry::get_damage_roll(ItemClass::GREAT_SWORD), "1d10");
}

// All Weapon Types Coverage
TEST_F(WeaponDamageRegistryTest, AllWeaponsHaveValidDamage) {
    // Comprehensive check: Every registered weapon must have valid damage
    std::vector<ItemClass> weapons = {
        ItemClass::DAGGER,
        ItemClass::SHORT_SWORD,
        ItemClass::LONG_SWORD,
        ItemClass::GREAT_SWORD,
        ItemClass::BATTLE_AXE,
        ItemClass::GREAT_AXE,
        ItemClass::WAR_HAMMER,
        ItemClass::MACE,
        ItemClass::STAFF,
        ItemClass::LONG_BOW,
        ItemClass::SHORT_BOW,
        ItemClass::CROSSBOW
    };

    for (ItemClass weapon : weapons) {
        if (WeaponDamageRegistry::is_registered(weapon)) {
            DamageInfo info = WeaponDamageRegistry::get_damage_info(weapon);

            EXPECT_TRUE(info.is_valid())
                << "Invalid damage for weapon class " << static_cast<int>(weapon);

            EXPECT_GT(info.maxDamage, 0)
                << "Zero max damage for weapon class " << static_cast<int>(weapon);

            EXPECT_GE(info.minDamage, 1)
                << "Invalid min damage for weapon class " << static_cast<int>(weapon);

            EXPECT_FALSE(info.displayRoll.empty())
                << "Missing display roll for weapon class " << static_cast<int>(weapon);
        }
    }
}

// Regression: Verify No Duplicate Damage Values
TEST_F(WeaponDamageRegistryTest, Regression_UniqueWeaponDamage) {
    // Each weapon type should have its own identity
    DamageInfo dagger = WeaponDamageRegistry::get_damage_info(ItemClass::DAGGER);
    DamageInfo longsword = WeaponDamageRegistry::get_damage_info(ItemClass::LONG_SWORD);
    DamageInfo greatsword = WeaponDamageRegistry::get_damage_info(ItemClass::GREAT_SWORD);

    // Dagger < Longsword < Greatsword in max damage
    EXPECT_LT(dagger.maxDamage, longsword.maxDamage);
    EXPECT_LT(longsword.maxDamage, greatsword.maxDamage);
}

// AD&D 2e Compliance
TEST_F(WeaponDamageRegistryTest, ADD2E_Compliance_WarHammer) {
    // War hammer in AD&D 2e: 1d4+1 (min 2, max 5)
    DamageInfo warhammer = WeaponDamageRegistry::get_damage_info(ItemClass::WAR_HAMMER);

    EXPECT_EQ(warhammer.minDamage, 2);
    EXPECT_EQ(warhammer.maxDamage, 5);
    EXPECT_EQ(warhammer.displayRoll, "1d4+1");
}

TEST_F(WeaponDamageRegistryTest, ADD2E_Compliance_TwoHandedSword) {
    // Great sword (2H sword) in AD&D 2e: 1d10
    DamageInfo greatsword = WeaponDamageRegistry::get_damage_info(ItemClass::GREAT_SWORD);

    EXPECT_EQ(greatsword.minDamage, 1);
    EXPECT_EQ(greatsword.maxDamage, 10);
}

// Performance: Lookup Speed
TEST_F(WeaponDamageRegistryTest, Performance_FastLookup) {
    // Verify lookups are fast (no heavy computation)
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        WeaponDamageRegistry::get_damage_info(ItemClass::LONG_SWORD);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 10,000 lookups should complete in under 100ms (conservative)
    EXPECT_LT(duration.count(), 100)
        << "Weapon lookups too slow: " << duration.count() << "ms for 10k lookups";
}
