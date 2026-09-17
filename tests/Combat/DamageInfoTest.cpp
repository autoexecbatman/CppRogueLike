#include <gtest/gtest.h>

#include <stdexcept>
#include "src/Combat/DamageInfo.h"
#include "src/Random/RandomDice.h"

// ============================================================================
// THE WIZARD'S DAMAGE SYSTEM TESTS
// Testing core combat calculations - if these fail, combat is broken
// ============================================================================

class DamageInfoTest : public ::testing::Test {
protected:
    RandomDice dice;
    DamageInfo dagger{"1d4", DamageType::PHYSICAL};
    DamageInfo longsword{"1d8", DamageType::PHYSICAL};
    DamageInfo warhammer{"1d4+1", DamageType::PHYSICAL};
};

// Basic Construction
TEST_F(DamageInfoTest, DefaultConstructor) {
    DamageInfo unarmed;
    EXPECT_EQ(unarmed.minDamage, 1);
    EXPECT_EQ(unarmed.maxDamage, 2);
    EXPECT_EQ(unarmed.displayRoll, "1d2");
}

TEST_F(DamageInfoTest, ParameterizedConstructor) {
    DamageInfo custom("1d6+4", DamageType::PHYSICAL);
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
    DamageInfo fixed("5", DamageType::PHYSICAL);
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
    sword = sword.with_enhancement(3);

    EXPECT_EQ(sword.minDamage, 4);   // 1 + 3
    EXPECT_EQ(sword.maxDamage, 11);  // 8 + 3
    EXPECT_EQ(sword.displayRoll, "1d8+3");
}

TEST_F(DamageInfoTest, AddBonus_Negative) {
    DamageInfo sword = longsword;
    sword = sword.with_enhancement(-2);

    EXPECT_EQ(sword.minDamage, -1);  // 1 - 2
    EXPECT_EQ(sword.maxDamage, 6);   // 8 - 2
    EXPECT_EQ(sword.displayRoll, "1d8-2");
}

TEST_F(DamageInfoTest, AddBonus_Zero) {
    DamageInfo sword = longsword;
    sword = sword.with_enhancement(0);

    EXPECT_EQ(sword.minDamage, 1);
    EXPECT_EQ(sword.maxDamage, 8);
    EXPECT_EQ(sword.displayRoll, "1d8");  // No change
}

TEST_F(DamageInfoTest, AddBonus_Chaining) {
    DamageInfo sword = longsword;
    sword = sword.with_enhancement(2);
    sword = sword.with_enhancement(3);

    EXPECT_EQ(sword.minDamage, 6);   // 1 + 2 + 3
    EXPECT_EQ(sword.maxDamage, 13);  // 8 + 2 + 3
}

TEST_F(DamageInfoTest, WithEnhancement_NonMutating) {
    DamageInfo original = longsword;
    DamageInfo enhanced = original.with_enhancement(3);

    // Original unchanged
    EXPECT_EQ(original.minDamage, 1);
    EXPECT_EQ(original.maxDamage, 8);

    // Enhanced has bonus
    EXPECT_EQ(enhanced.minDamage, 4);
    EXPECT_EQ(enhanced.maxDamage, 11);
}

// Validation
TEST_F(DamageInfoTest, IsValid_NormalCases) {
    EXPECT_TRUE(dagger.is_valid());
    EXPECT_TRUE(longsword.is_valid());
    EXPECT_TRUE(warhammer.is_valid());
}

// The range is derived from the dice, so a DamageInfo that is not valid cannot
// be built: the constructor refuses dice that roll nothing or less than nothing.
TEST_F(DamageInfoTest, IsValid_EdgeCases) {
    EXPECT_TRUE(DamageInfo("1", DamageType::PHYSICAL).is_valid());     // A fixed value
    EXPECT_TRUE(DamageInfo("1d4", DamageType::PHYSICAL).is_valid());
    EXPECT_THROW(DamageInfo("0d5", DamageType::PHYSICAL), std::invalid_argument);  // No dice to roll
    EXPECT_THROW(DamageInfo("5d0", DamageType::PHYSICAL), std::invalid_argument);  // No sides
    EXPECT_THROW(DamageInfo("-1d5", DamageType::PHYSICAL), std::invalid_argument); // A negative count
}

// Display
TEST_F(DamageInfoTest, GetDamageRange_Variable) {
    EXPECT_EQ(dagger.get_damage_range(), "1-4");
    EXPECT_EQ(longsword.get_damage_range(), "1-8");
    EXPECT_EQ(warhammer.get_damage_range(), "2-5");
}

TEST_F(DamageInfoTest, GetDamageRange_Fixed) {
    DamageInfo fixed("5", DamageType::PHYSICAL);
    EXPECT_EQ(fixed.get_damage_range(), "5");
}

// Equality
TEST_F(DamageInfoTest, Equality_SameValues) {
    DamageInfo sword1("1d8", DamageType::PHYSICAL);
    DamageInfo sword2("1d8", DamageType::PHYSICAL);

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

    EXPECT_EQ(dagger_ref, DamageInfo("1d4", DamageType::PHYSICAL));
    EXPECT_EQ(longsword_ref, DamageInfo("1d8", DamageType::PHYSICAL));
    EXPECT_EQ(greatsword_ref, DamageInfo("1d10", DamageType::PHYSICAL));
}

// Regression Test: Strength Bonus Application
TEST_F(DamageInfoTest, Regression_StrengthBonus_AddedCorrectly) {
    // Bug scenario: Strength bonus not applying to damage
    DamageInfo base_dagger = DamageValues::Dagger();
    int strength_bonus = 2;  // 18 Strength = +2 damage

    DamageInfo enhanced = base_dagger.with_enhancement(strength_bonus);

    EXPECT_EQ(enhanced.minDamage, 3);  // 1 + 2
    EXPECT_EQ(enhanced.maxDamage, 6);  // 4 + 2

    // Verify roll respects new range
    for (int i = 0; i < 100; ++i) {
        int damage = enhanced.roll_damage(&dice);
        EXPECT_GE(damage, 3);
        EXPECT_LE(damage, 6);
    }
}

// Edge Case: Maximum Enhancement Stacking
TEST_F(DamageInfoTest, EdgeCase_MassiveEnhancementStack) {
    DamageInfo weapon = DamageValues::Dagger();

    // Stack +1, +2, +3, +4, +5 bonuses
    weapon = weapon.with_enhancement(1);
    weapon = weapon.with_enhancement(2);
    weapon = weapon.with_enhancement(3);
    weapon = weapon.with_enhancement(4);
    weapon = weapon.with_enhancement(5);

    EXPECT_EQ(weapon.minDamage, 16);  // 1 + (1+2+3+4+5)
    EXPECT_EQ(weapon.maxDamage, 19);  // 4 + (1+2+3+4+5)
    EXPECT_EQ(weapon.displayRoll, "1d4+15") << "five bonuses are one sum, not five of them";
    EXPECT_NO_THROW(parse_dice_expression(weapon.displayRoll));
}

// The dice are the record, so a mutation moves them and the range and the text
// are re-derived. Building the text by appending to it instead produces
// "1d8+2+3", which parse_dice_expression refuses - a DamageInfo that can no
// longer be read back from the file it would be written to.
TEST_F(DamageInfoTest, ABonusAddedTwiceStillReadsAsDice)
{
    DamageInfo weapon{ "1d8", DamageType::PHYSICAL };

    weapon = weapon.with_enhancement(2);
    weapon = weapon.with_enhancement(3);

    EXPECT_EQ(weapon.dice.bonus, 5);
    EXPECT_EQ(weapon.displayRoll, "1d8+5");
    EXPECT_NO_THROW(parse_dice_expression(weapon.displayRoll));
    EXPECT_EQ(weapon.minDamage, 6);
    EXPECT_EQ(weapon.maxDamage, 13);
}

// A bonus that cancels leaves the plain dice behind, not a trailing "+0".
TEST_F(DamageInfoTest, ABonusThatCancelsLeavesThePlainDice)
{
    DamageInfo weapon{ "1d8", DamageType::PHYSICAL };

    weapon = weapon.with_enhancement(2);
    weapon = weapon.with_enhancement(-2);

    EXPECT_EQ(weapon.displayRoll, "1d8");
    EXPECT_EQ(weapon.minDamage, 1);
    EXPECT_EQ(weapon.maxDamage, 8);
}

// A bonus changes how hard the weapon hits, never what kind of damage it deals.
// The type is what resistance reads, so a flaming sword that lost it on being
// enhanced would pass straight through fire resistance.
TEST_F(DamageInfoTest, ABonusKeepsTheDamageType)
{
    DamageInfo flaming{ "1d8", DamageType::FIRE };

    flaming = flaming.with_enhancement(2);

    EXPECT_EQ(flaming.damageType, DamageType::FIRE);
    const DamageInfo frost{ "1d6", DamageType::COLD };
    EXPECT_EQ(frost.with_enhancement(3).damageType, DamageType::COLD);
}
