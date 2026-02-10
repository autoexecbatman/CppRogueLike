#include <gtest/gtest.h>
#include "src/Combat/DamageInfo.h"
#include "src/Random/RandomDice.h"

// ============================================================================
// THE WIZARD'S DAMAGE SYSTEM TESTS
// Testing core combat calculations - if these fail, combat is broken
// ============================================================================

class DamageInfoTest : public ::testing::Test {
protected:
    RandomDice dice;
    DamageInfo dagger{1, 4, "1d4"};
    DamageInfo longsword{1, 8, "1d8"};
    DamageInfo warhammer{2, 5, "1d4+1"};
};

// Basic Construction
TEST_F(DamageInfoTest, DefaultConstructor) {
    DamageInfo unarmed;
    EXPECT_EQ(unarmed.minDamage, 1);
    EXPECT_EQ(unarmed.maxDamage, 2);
    EXPECT_EQ(unarmed.displayRoll, "1d2");
}

TEST_F(DamageInfoTest, ParameterizedConstructor) {
    DamageInfo custom(5, 10, "1d6+4");
    EXPECT_EQ(custom.minDamage, 5);
    EXPECT_EQ(custom.maxDamage, 10);
    EXPECT_EQ(custom.displayRoll, "1d6+4");
}

// Damage Rolling
TEST_F(DamageInfoTest, RollDamage_RespectsMinMax) {
    // Roll 1000 times, all results must be in valid range
    for (int i = 0; i < 1000; ++i) {
        int damage = longsword.roll_damage(&dice);
        EXPECT_GE(damage, longsword.minDamage) << "Damage below minimum on roll " << i;
        EXPECT_LE(damage, longsword.maxDamage) << "Damage above maximum on roll " << i;
    }
}

TEST_F(DamageInfoTest, RollDamage_FixedValue) {
    DamageInfo fixed(5, 5, "5");
    EXPECT_EQ(fixed.roll_damage(&dice), 5);
    EXPECT_EQ(fixed.roll_damage(&dice), 5);
    EXPECT_EQ(fixed.roll_damage(&dice), 5);
}

TEST_F(DamageInfoTest, GetAverageDamage) {
    EXPECT_EQ(dagger.get_average_damage(), 2);        // (1+4)/2 = 2
    EXPECT_EQ(longsword.get_average_damage(), 4);     // (1+8)/2 = 4
    EXPECT_EQ(warhammer.get_average_damage(), 3);     // (2+5)/2 = 3
}

// Damage Modification
TEST_F(DamageInfoTest, AddBonus_Positive) {
    DamageInfo sword = longsword;  // Copy
    sword.add_bonus(3);

    EXPECT_EQ(sword.minDamage, 4);   // 1 + 3
    EXPECT_EQ(sword.maxDamage, 11);  // 8 + 3
    EXPECT_EQ(sword.displayRoll, "1d8+3");
}

TEST_F(DamageInfoTest, AddBonus_Negative) {
    DamageInfo sword = longsword;
    sword.add_bonus(-2);

    EXPECT_EQ(sword.minDamage, -1);  // 1 - 2
    EXPECT_EQ(sword.maxDamage, 6);   // 8 - 2
    EXPECT_EQ(sword.displayRoll, "1d8-2");
}

TEST_F(DamageInfoTest, AddBonus_Zero) {
    DamageInfo sword = longsword;
    sword.add_bonus(0);

    EXPECT_EQ(sword.minDamage, 1);
    EXPECT_EQ(sword.maxDamage, 8);
    EXPECT_EQ(sword.displayRoll, "1d8");  // No change
}

TEST_F(DamageInfoTest, AddBonus_Chaining) {
    DamageInfo sword = longsword;
    sword.add_bonus(2).add_bonus(3);

    EXPECT_EQ(sword.minDamage, 6);   // 1 + 2 + 3
    EXPECT_EQ(sword.maxDamage, 13);  // 8 + 2 + 3
}

TEST_F(DamageInfoTest, WithEnhancement_NonMutating) {
    DamageInfo original = longsword;
    DamageInfo enhanced = original.with_enhancement(3, 0);

    // Original unchanged
    EXPECT_EQ(original.minDamage, 1);
    EXPECT_EQ(original.maxDamage, 8);

    // Enhanced has bonus
    EXPECT_EQ(enhanced.minDamage, 4);
    EXPECT_EQ(enhanced.maxDamage, 11);
}

TEST_F(DamageInfoTest, MultiplyDamage) {
    DamageInfo sword = longsword;
    sword.multiply_damage(2.0f);

    EXPECT_EQ(sword.minDamage, 2);   // 1 * 2
    EXPECT_EQ(sword.maxDamage, 16);  // 8 * 2
}

TEST_F(DamageInfoTest, MultiplyDamage_Fractional) {
    DamageInfo sword = longsword;
    sword.multiply_damage(0.5f);

    EXPECT_EQ(sword.minDamage, 0);   // 1 * 0.5 = 0 (truncated)
    EXPECT_EQ(sword.maxDamage, 4);   // 8 * 0.5 = 4
}

// Validation
TEST_F(DamageInfoTest, IsValid_NormalCases) {
    EXPECT_TRUE(dagger.is_valid());
    EXPECT_TRUE(longsword.is_valid());
    EXPECT_TRUE(warhammer.is_valid());
}

TEST_F(DamageInfoTest, IsValid_EdgeCases) {
    EXPECT_TRUE(DamageInfo(1, 1, "1").is_valid());     // Equal min/max
    EXPECT_FALSE(DamageInfo(0, 5, "0d5").is_valid()); // Zero min
    EXPECT_FALSE(DamageInfo(5, 3, "5d3").is_valid()); // Min > max
    EXPECT_FALSE(DamageInfo(-1, 5, "-1d5").is_valid()); // Negative min
}

// Display
TEST_F(DamageInfoTest, GetDamageRange_Variable) {
    EXPECT_EQ(dagger.get_damage_range(), "1-4");
    EXPECT_EQ(longsword.get_damage_range(), "1-8");
    EXPECT_EQ(warhammer.get_damage_range(), "2-5");
}

TEST_F(DamageInfoTest, GetDamageRange_Fixed) {
    DamageInfo fixed(5, 5, "5");
    EXPECT_EQ(fixed.get_damage_range(), "5");
}

// Equality
TEST_F(DamageInfoTest, Equality_SameValues) {
    DamageInfo sword1(1, 8, "1d8");
    DamageInfo sword2(1, 8, "1d8");

    EXPECT_EQ(sword1, sword2);
    EXPECT_FALSE(sword1 != sword2);
}

TEST_F(DamageInfoTest, Equality_DifferentValues) {
    EXPECT_NE(dagger, longsword);
    EXPECT_FALSE(dagger == longsword);
}

// Namespace Helpers
TEST_F(DamageInfoTest, DamageValues_Unarmed) {
    auto unarmed = DamageValues::Unarmed();
    EXPECT_EQ(unarmed.minDamage, 1);
    EXPECT_EQ(unarmed.maxDamage, 2);
    EXPECT_EQ(unarmed.displayRoll, "1d2");
}

TEST_F(DamageInfoTest, DamageValues_CommonWeapons) {
    auto dagger_ref = DamageValues::Dagger();
    auto longsword_ref = DamageValues::LongSword();
    auto greatsword_ref = DamageValues::GreatSword();

    EXPECT_EQ(dagger_ref, DamageInfo(1, 4, "1d4"));
    EXPECT_EQ(longsword_ref, DamageInfo(1, 8, "1d8"));
    EXPECT_EQ(greatsword_ref, DamageInfo(1, 10, "1d10"));
}

// Regression Test: Strength Bonus Application
TEST_F(DamageInfoTest, Regression_StrengthBonus_AddedCorrectly) {
    // Bug scenario: Strength bonus not applying to damage
    DamageInfo base_dagger = DamageValues::Dagger();
    int strength_bonus = 2;  // 18 Strength = +2 damage

    DamageInfo enhanced = base_dagger.with_enhancement(strength_bonus, 0);

    EXPECT_EQ(enhanced.minDamage, 3);  // 1 + 2
    EXPECT_EQ(enhanced.maxDamage, 6);  // 4 + 2

    // Verify roll respects new range
    for (int i = 0; i < 100; ++i) {
        int damage = enhanced.roll_damage(&dice);
        EXPECT_GE(damage, 3);
        EXPECT_LE(damage, 6);
    }
}

// Critical Hit Scenario
TEST_F(DamageInfoTest, CriticalHit_DoubledDamage) {
    DamageInfo base = longsword;
    DamageInfo critical = base;
    critical.multiply_damage(2.0f);

    EXPECT_EQ(critical.minDamage, 2);   // 1 * 2
    EXPECT_EQ(critical.maxDamage, 16);  // 8 * 2
}

// Edge Case: Maximum Enhancement Stacking
TEST_F(DamageInfoTest, EdgeCase_MassiveEnhancementStack) {
    DamageInfo weapon = DamageValues::Dagger();

    // Stack +1, +2, +3, +4, +5 bonuses
    weapon.add_bonus(1)
          .add_bonus(2)
          .add_bonus(3)
          .add_bonus(4)
          .add_bonus(5);

    EXPECT_EQ(weapon.minDamage, 16);  // 1 + (1+2+3+4+5)
    EXPECT_EQ(weapon.maxDamage, 19);  // 4 + (1+2+3+4+5)
}
