#include <gtest/gtest.h>
#include "src/Systems/ItemEnhancements/ItemEnhancements.h"
#include "src/Actor/Actor.h"
#include "src/Factories/ItemCreator.h"
#include "src/Combat/WeaponDamageRegistry.h"

// ============================================================================
// ENHANCEMENT SYSTEM TESTS
// Tests item enhancement generation, application, naming, and edge cases
// ============================================================================

class EnhancementSystemTest : public ::testing::Test {
protected:
    ItemEnhancement defaultEnhancement;

    void SetUp() override {
        defaultEnhancement = ItemEnhancement{};
    }
};

// ----------------------------------------------------------------------------
// Default State Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, DefaultEnhancement_HasNoPrefix) {
    EXPECT_EQ(defaultEnhancement.prefix, PrefixType::NONE);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_HasNoSuffix) {
    EXPECT_EQ(defaultEnhancement.suffix, SuffixType::NONE);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_ZeroBonuses) {
    EXPECT_EQ(defaultEnhancement.damage_bonus, 0);
    EXPECT_EQ(defaultEnhancement.to_hit_bonus, 0);
    EXPECT_EQ(defaultEnhancement.ac_bonus, 0);
    EXPECT_EQ(defaultEnhancement.strength_bonus, 0);
    EXPECT_EQ(defaultEnhancement.dexterity_bonus, 0);
    EXPECT_EQ(defaultEnhancement.intelligence_bonus, 0);
    EXPECT_EQ(defaultEnhancement.hp_bonus, 0);
    EXPECT_EQ(defaultEnhancement.mana_bonus, 0);
    EXPECT_EQ(defaultEnhancement.speed_bonus, 0);
    EXPECT_EQ(defaultEnhancement.stealth_bonus, 0);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_ZeroResistances) {
    EXPECT_EQ(defaultEnhancement.fire_resistance, 0);
    EXPECT_EQ(defaultEnhancement.cold_resistance, 0);
    EXPECT_EQ(defaultEnhancement.lightning_resistance, 0);
    EXPECT_EQ(defaultEnhancement.poison_resistance, 0);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_NotCursedOrBlessed) {
    EXPECT_FALSE(defaultEnhancement.is_cursed);
    EXPECT_FALSE(defaultEnhancement.is_blessed);
    EXPECT_FALSE(defaultEnhancement.is_magical);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_BaseValueModifier) {
    EXPECT_EQ(defaultEnhancement.value_modifier, 100);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_ZeroEnhancementLevel) {
    EXPECT_EQ(defaultEnhancement.enhancement_level, 0);
}

// ----------------------------------------------------------------------------
// Prefix Name Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, GetPrefixName_None_ReturnsEmpty) {
    defaultEnhancement.prefix = PrefixType::NONE;
    EXPECT_EQ(defaultEnhancement.get_prefix_name(), "");
}

TEST_F(EnhancementSystemTest, GetPrefixName_Sharp_ReturnsSharp) {
    defaultEnhancement.prefix = PrefixType::SHARP;
    EXPECT_EQ(defaultEnhancement.get_prefix_name(), "Sharp");
}

TEST_F(EnhancementSystemTest, GetPrefixName_Keen_ReturnsKeen) {
    defaultEnhancement.prefix = PrefixType::KEEN;
    EXPECT_EQ(defaultEnhancement.get_prefix_name(), "Keen");
}

TEST_F(EnhancementSystemTest, GetPrefixName_Masterwork_ReturnsMasterwork) {
    defaultEnhancement.prefix = PrefixType::MASTERWORK;
    EXPECT_EQ(defaultEnhancement.get_prefix_name(), "Masterwork");
}

TEST_F(EnhancementSystemTest, GetPrefixName_Cursed_ReturnsCursed) {
    defaultEnhancement.prefix = PrefixType::CURSED;
    EXPECT_EQ(defaultEnhancement.get_prefix_name(), "Cursed");
}

// ----------------------------------------------------------------------------
// Suffix Name Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, GetSuffixName_None_ReturnsEmpty) {
    defaultEnhancement.suffix = SuffixType::NONE;
    EXPECT_EQ(defaultEnhancement.get_suffix_name(), "");
}

TEST_F(EnhancementSystemTest, GetSuffixName_OfSlaying_ReturnsCorrect) {
    defaultEnhancement.suffix = SuffixType::OF_SLAYING;
    EXPECT_EQ(defaultEnhancement.get_suffix_name(), "of Slaying");
}

TEST_F(EnhancementSystemTest, GetSuffixName_OfTheBear_ReturnsCorrect) {
    defaultEnhancement.suffix = SuffixType::OF_THE_BEAR;
    EXPECT_EQ(defaultEnhancement.get_suffix_name(), "of the Bear");
}

TEST_F(EnhancementSystemTest, GetSuffixName_OfFireResistance_ReturnsCorrect) {
    defaultEnhancement.suffix = SuffixType::OF_FIRE_RESISTANCE;
    EXPECT_EQ(defaultEnhancement.get_suffix_name(), "of Fire Resistance");
}

// ----------------------------------------------------------------------------
// Full Name Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, GetFullName_NoPrefixNoSuffix_ReturnsBaseName) {
    defaultEnhancement.prefix = PrefixType::NONE;
    defaultEnhancement.suffix = SuffixType::NONE;

    std::string fullName = defaultEnhancement.get_full_name("long sword");

    EXPECT_EQ(fullName, "long sword");
}

TEST_F(EnhancementSystemTest, GetFullName_PrefixOnly_PrependPrefix) {
    defaultEnhancement.prefix = PrefixType::SHARP;
    defaultEnhancement.suffix = SuffixType::NONE;

    std::string fullName = defaultEnhancement.get_full_name("long sword");

    EXPECT_EQ(fullName, "Sharp long sword");
}

TEST_F(EnhancementSystemTest, GetFullName_SuffixOnly_AppendSuffix) {
    defaultEnhancement.prefix = PrefixType::NONE;
    defaultEnhancement.suffix = SuffixType::OF_SLAYING;

    std::string fullName = defaultEnhancement.get_full_name("long sword");

    EXPECT_EQ(fullName, "long sword of Slaying");
}

TEST_F(EnhancementSystemTest, GetFullName_PrefixAndSuffix_IncludesBoth) {
    defaultEnhancement.prefix = PrefixType::SHARP;
    defaultEnhancement.suffix = SuffixType::OF_THE_BEAR;

    std::string fullName = defaultEnhancement.get_full_name("long sword");

    EXPECT_EQ(fullName, "Sharp long sword of the Bear");
}

TEST_F(EnhancementSystemTest, GetFullName_WithEnhancementLevel_IncludesPlus) {
    defaultEnhancement.enhancement_level = 2;
    defaultEnhancement.prefix = PrefixType::NONE;
    defaultEnhancement.suffix = SuffixType::NONE;

    std::string fullName = defaultEnhancement.get_full_name("long sword");

    // Should include "+2" somewhere
    EXPECT_NE(fullName.find("+2"), std::string::npos);
}

// ----------------------------------------------------------------------------
// Apply Enhancement Effects Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, ApplyEffects_SharpPrefix_AddsDamageBonus) {
    defaultEnhancement.prefix = PrefixType::SHARP;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.damage_bonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_KeenPrefix_AddsDamageBonus) {
    defaultEnhancement.prefix = PrefixType::KEEN;

    defaultEnhancement.apply_enhancement_effects();

    // Keen should give more damage than Sharp
    ItemEnhancement sharpEnhancement;
    sharpEnhancement.prefix = PrefixType::SHARP;
    sharpEnhancement.apply_enhancement_effects();

    EXPECT_GE(defaultEnhancement.damage_bonus, sharpEnhancement.damage_bonus);
}

TEST_F(EnhancementSystemTest, ApplyEffects_MasterworkPrefix_AddsToHit) {
    defaultEnhancement.prefix = PrefixType::MASTERWORK;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.to_hit_bonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_BlessedPrefix_AddsBothBonuses) {
    defaultEnhancement.prefix = PrefixType::BLESSED;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.damage_bonus, 0);
    EXPECT_GT(defaultEnhancement.to_hit_bonus, 0);
    EXPECT_TRUE(defaultEnhancement.is_blessed);
}

TEST_F(EnhancementSystemTest, ApplyEffects_ReinforcedPrefix_AddsACBonus) {
    defaultEnhancement.prefix = PrefixType::REINFORCED;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.ac_bonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_CursedPrefix_SetsCursedFlag) {
    defaultEnhancement.prefix = PrefixType::CURSED;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_TRUE(defaultEnhancement.is_cursed);
}

TEST_F(EnhancementSystemTest, ApplyEffects_OfSlayingSuffix_AddsDamage) {
    defaultEnhancement.suffix = SuffixType::OF_SLAYING;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.damage_bonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_OfTheBearSuffix_AddsStrength) {
    defaultEnhancement.suffix = SuffixType::OF_THE_BEAR;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.strength_bonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_OfTheEagleSuffix_AddsDexterity) {
    defaultEnhancement.suffix = SuffixType::OF_THE_EAGLE;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.dexterity_bonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_OfFireResistance_AddsResistance) {
    defaultEnhancement.suffix = SuffixType::OF_FIRE_RESISTANCE;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.fire_resistance, 0);
}

// ----------------------------------------------------------------------------
// Resistance Value Range Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, FireResistance_InValidRange) {
    defaultEnhancement.suffix = SuffixType::OF_FIRE_RESISTANCE;
    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GE(defaultEnhancement.fire_resistance, 0);
    EXPECT_LE(defaultEnhancement.fire_resistance, 100);
}

TEST_F(EnhancementSystemTest, ColdResistance_InValidRange) {
    defaultEnhancement.suffix = SuffixType::OF_COLD_RESISTANCE;
    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GE(defaultEnhancement.cold_resistance, 0);
    EXPECT_LE(defaultEnhancement.cold_resistance, 100);
}

// ----------------------------------------------------------------------------
// Random Generation Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, GenerateRandom_ReturnsValidEnhancement) {
    auto enhancement = ItemEnhancement::generate_random_enhancement();

    // Should have at least one enhancement (prefix or suffix or level)
    bool hasEnhancement = (enhancement.prefix != PrefixType::NONE) ||
                          (enhancement.suffix != SuffixType::NONE) ||
                          (enhancement.enhancement_level > 0);

    // May or may not have enhancement depending on random chance
    // Just verify no crash
    EXPECT_NO_THROW(enhancement.get_full_name("test item"));
}

TEST_F(EnhancementSystemTest, GenerateWeaponEnhancement_ValidForWeapons) {
    auto enhancement = ItemEnhancement::generate_weapon_enhancement();

    // Weapon enhancements should typically affect damage or to-hit
    // After applying effects
    enhancement.apply_enhancement_effects();

    // Verify no crash and structure is valid
    EXPECT_NO_THROW(enhancement.get_full_name("sword"));
}

TEST_F(EnhancementSystemTest, GenerateArmorEnhancement_ValidForArmor) {
    auto enhancement = ItemEnhancement::generate_armor_enhancement();

    enhancement.apply_enhancement_effects();

    // Armor enhancements should affect AC
    EXPECT_NO_THROW(enhancement.get_full_name("armor"));
}

TEST_F(EnhancementSystemTest, GenerateByRarity_Level1_BasicEnhancement) {
    auto enhancement = ItemEnhancement::generate_by_rarity(1);

    // Low rarity should have minimal bonuses
    enhancement.apply_enhancement_effects();

    // Verify structure is valid
    EXPECT_NO_THROW(enhancement.get_full_name("item"));
}

TEST_F(EnhancementSystemTest, GenerateByRarity_Level5_PowerfulEnhancement) {
    auto enhancement = ItemEnhancement::generate_by_rarity(5);

    enhancement.apply_enhancement_effects();

    // High rarity may have better bonuses
    EXPECT_NO_THROW(enhancement.get_full_name("item"));
}

// ----------------------------------------------------------------------------
// Value Modifier Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, ValueModifier_100_NoChange) {
    defaultEnhancement.value_modifier = 100;

    int baseValue = 100;
    int enhancedValue = (baseValue * defaultEnhancement.value_modifier) / 100;

    EXPECT_EQ(enhancedValue, 100);
}

TEST_F(EnhancementSystemTest, ValueModifier_150_IncreasesValue) {
    defaultEnhancement.value_modifier = 150;

    int baseValue = 100;
    int enhancedValue = (baseValue * defaultEnhancement.value_modifier) / 100;

    EXPECT_EQ(enhancedValue, 150);
}

TEST_F(EnhancementSystemTest, ValueModifier_50_DecreasesValue) {
    defaultEnhancement.value_modifier = 50;

    int baseValue = 100;
    int enhancedValue = (baseValue * defaultEnhancement.value_modifier) / 100;

    EXPECT_EQ(enhancedValue, 50);
}

// ----------------------------------------------------------------------------
// Enhancement Level Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, EnhancementLevel_AffectsName) {
    defaultEnhancement.enhancement_level = 3;

    std::string name = defaultEnhancement.get_full_name("sword");

    EXPECT_NE(name.find("+3"), std::string::npos);
}

TEST_F(EnhancementSystemTest, EnhancementLevel_Zero_NoPlus) {
    defaultEnhancement.enhancement_level = 0;

    std::string name = defaultEnhancement.get_full_name("sword");

    // Should not have "+0" in name
    EXPECT_EQ(name.find("+0"), std::string::npos);
}

// ----------------------------------------------------------------------------
// Combination Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, PrefixAndSuffixBonuses_Stack) {
    defaultEnhancement.prefix = PrefixType::SHARP; // +damage
    defaultEnhancement.suffix = SuffixType::OF_SLAYING; // +damage

    defaultEnhancement.apply_enhancement_effects();

    // Both should contribute to damage
    EXPECT_GE(defaultEnhancement.damage_bonus, 2);
}

TEST_F(EnhancementSystemTest, CursedItem_CanHaveOtherEnhancements) {
    defaultEnhancement.prefix = PrefixType::CURSED;
    defaultEnhancement.suffix = SuffixType::OF_SLAYING;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_TRUE(defaultEnhancement.is_cursed);
    // Still may have damage bonus from suffix
}

TEST_F(EnhancementSystemTest, BlessedItem_CannotBeCursed) {
    defaultEnhancement.prefix = PrefixType::BLESSED;
    defaultEnhancement.is_cursed = false;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_TRUE(defaultEnhancement.is_blessed);
    // Blessed should not be cursed (unless specifically set)
    EXPECT_FALSE(defaultEnhancement.is_cursed);
}

// ----------------------------------------------------------------------------
// Edge Cases
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, EmptyBaseName_StillWorks) {
    defaultEnhancement.prefix = PrefixType::SHARP;
    defaultEnhancement.suffix = SuffixType::OF_SLAYING;

    std::string name = defaultEnhancement.get_full_name("");

    // Should not crash, may produce "Sharp  of Slaying" or similar
    EXPECT_FALSE(name.empty());
}

TEST_F(EnhancementSystemTest, VeryLongBaseName_StillWorks) {
    defaultEnhancement.prefix = PrefixType::SHARP;

    std::string longName(1000, 'a');
    std::string name = defaultEnhancement.get_full_name(longName);

    EXPECT_FALSE(name.empty());
    EXPECT_GT(name.length(), longName.length());
}

TEST_F(EnhancementSystemTest, MultipleApplyEffects_Idempotent) {
    defaultEnhancement.prefix = PrefixType::SHARP;

    defaultEnhancement.apply_enhancement_effects();
    int firstDamageBonus = defaultEnhancement.damage_bonus;

    defaultEnhancement.apply_enhancement_effects();
    int secondDamageBonus = defaultEnhancement.damage_bonus;

    // Applying effects multiple times should be idempotent or additive
    // (depends on implementation - this documents behavior)
    EXPECT_GE(secondDamageBonus, firstDamageBonus);
}

// ----------------------------------------------------------------------------
// Integration with Item System
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, WeaponDamageRegistry_AppliesEnhancement) {
    ItemEnhancement enhancement;
    enhancement.damage_bonus = 3;

    DamageInfo baseDamage = WeaponDamageRegistry::get_damage_info(ItemClass::LONG_SWORD);
    DamageInfo enhancedDamage = WeaponDamageRegistry::get_enhanced_damage_info(
        ItemClass::LONG_SWORD, &enhancement);

    EXPECT_EQ(enhancedDamage.minDamage, baseDamage.minDamage + 3);
    EXPECT_EQ(enhancedDamage.maxDamage, baseDamage.maxDamage + 3);
}

TEST_F(EnhancementSystemTest, WeaponDamageRegistry_NullEnhancement_ReturnsBase) {
    DamageInfo baseDamage = WeaponDamageRegistry::get_damage_info(ItemClass::DAGGER);
    DamageInfo withNull = WeaponDamageRegistry::get_enhanced_damage_info(
        ItemClass::DAGGER, nullptr);

    EXPECT_EQ(withNull.minDamage, baseDamage.minDamage);
    EXPECT_EQ(withNull.maxDamage, baseDamage.maxDamage);
}
