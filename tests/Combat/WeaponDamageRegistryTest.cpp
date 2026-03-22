#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <vector>
#include "src/Combat/WeaponDamageRegistry.h"
#include "src/Combat/DamageInfo.h"

// ============================================================================
// WEAPON DAMAGE REGISTRY TESTS
// Validates damage lookup system - core combat mechanic
// ============================================================================

class WeaponDamageRegistryTest : public ::testing::Test {
protected:
    // No setup needed - WeaponDamageRegistry is static
};

// Basic Lookups
TEST_F(WeaponDamageRegistryTest, GetDamageInfo_Dagger) {
    DamageInfo info = WeaponDamageRegistry::get_damage_info("dagger");

    EXPECT_EQ(info.minDamage, 1);
    EXPECT_EQ(info.maxDamage, 4);
    EXPECT_EQ(info.displayRoll, "1d4");
}

TEST_F(WeaponDamageRegistryTest, GetDamageInfo_LongSword) {
    DamageInfo info = WeaponDamageRegistry::get_damage_info("long_sword");

    EXPECT_EQ(info.minDamage, 1);
    EXPECT_EQ(info.maxDamage, 8);
    EXPECT_EQ(info.displayRoll, "1d8");
}

TEST_F(WeaponDamageRegistryTest, GetDamageInfo_GreatSword) {
    DamageInfo info = WeaponDamageRegistry::get_damage_info("great_sword");

    EXPECT_EQ(info.minDamage, 1);
    EXPECT_EQ(info.maxDamage, 10);
}

TEST_F(WeaponDamageRegistryTest, GetDamageInfo_BattleAxe) {
    DamageInfo info = WeaponDamageRegistry::get_damage_info("battle_axe");

    EXPECT_EQ(info.minDamage, 1);
    EXPECT_EQ(info.maxDamage, 8);
}

// Registration Status
TEST_F(WeaponDamageRegistryTest, IsRegistered_ValidWeapons) {
    EXPECT_TRUE(WeaponDamageRegistry::is_registered("dagger"));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered("short_sword"));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered("long_sword"));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered("great_sword"));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered("battle_axe"));
    EXPECT_TRUE(WeaponDamageRegistry::is_registered("war_hammer"));
}

TEST_F(WeaponDamageRegistryTest, IsRegistered_NonWeapons) {
    EXPECT_FALSE(WeaponDamageRegistry::is_registered("health_potion"));
    EXPECT_FALSE(WeaponDamageRegistry::is_registered("leather_armor"));
    EXPECT_FALSE(WeaponDamageRegistry::is_registered("scroll_lightning"));
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

// Display Strings
TEST_F(WeaponDamageRegistryTest, GetDamageRoll_DisplayStrings) {
    EXPECT_EQ(WeaponDamageRegistry::get_damage_roll("dagger"), "1d4");
    EXPECT_EQ(WeaponDamageRegistry::get_damage_roll("long_sword"), "1d8");
    EXPECT_EQ(WeaponDamageRegistry::get_damage_roll("great_sword"), "1d10");
}

// All Weapon Types Coverage
TEST_F(WeaponDamageRegistryTest, AllWeaponsHaveValidDamage) {
    std::vector<std::string> weapons = {
        "dagger",
        "short_sword",
        "long_sword",
        "great_sword",
        "battle_axe",
        "great_axe",
        "war_hammer",
        "mace",
        "staff",
        "long_bow",
        "short_bow",
        "light_crossbow"
    };

    for (const auto& weapon : weapons) {
        if (WeaponDamageRegistry::is_registered(weapon)) {
            DamageInfo info = WeaponDamageRegistry::get_damage_info(weapon);

            EXPECT_TRUE(info.is_valid())
                << "Invalid damage for weapon: " << weapon;

            EXPECT_GT(info.maxDamage, 0)
                << "Zero max damage for weapon: " << weapon;

            EXPECT_GE(info.minDamage, 1)
                << "Invalid min damage for weapon: " << weapon;

            EXPECT_FALSE(info.displayRoll.empty())
                << "Missing display roll for weapon: " << weapon;
        }
    }
}

// Regression: Verify Ordered Damage Values
TEST_F(WeaponDamageRegistryTest, Regression_UniqueWeaponDamage) {
    DamageInfo dagger = WeaponDamageRegistry::get_damage_info("dagger");
    DamageInfo longsword = WeaponDamageRegistry::get_damage_info("long_sword");
    DamageInfo greatsword = WeaponDamageRegistry::get_damage_info("great_sword");

    EXPECT_LT(dagger.maxDamage, longsword.maxDamage);
    EXPECT_LT(longsword.maxDamage, greatsword.maxDamage);
}

// AD&D 2e Compliance
TEST_F(WeaponDamageRegistryTest, ADD2E_Compliance_WarHammer) {
    DamageInfo warhammer = WeaponDamageRegistry::get_damage_info("war_hammer");

    EXPECT_EQ(warhammer.minDamage, 2);
    EXPECT_EQ(warhammer.maxDamage, 5);
    EXPECT_EQ(warhammer.displayRoll, "1d4+1");
}

TEST_F(WeaponDamageRegistryTest, ADD2E_Compliance_TwoHandedSword) {
    DamageInfo greatsword = WeaponDamageRegistry::get_damage_info("great_sword");

    EXPECT_EQ(greatsword.minDamage, 1);
    EXPECT_EQ(greatsword.maxDamage, 10);
}

// Performance: Lookup Speed
TEST_F(WeaponDamageRegistryTest, Performance_FastLookup) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        WeaponDamageRegistry::get_damage_info("long_sword");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_LT(duration.count(), 100)
        << "Weapon lookups too slow: " << duration.count() << "ms for 10k lookups";
}
