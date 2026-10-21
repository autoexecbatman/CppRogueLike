#include <gtest/gtest.h>
#include "src/ItemEnhancements.h"
#include "src/RandomDice.h"
#include "src/Actor.h"
#include "src/ItemCreator.h"
#include "src/WeaponDamageRegistry.h"
#include "tests/mocks/TestDice.h"

// ============================================================================
// ENHANCEMENT SYSTEM TESTS
// Tests item enhancement generation, application, naming, and edge cases
// ============================================================================

class EnhancementSystemTest : public ::testing::Test {
protected:
    ItemEnhancement defaultEnhancement;
    // The generators draw from this rather than from rand(), so a test can queue
    // the rolls it wants. Left out of test mode except where a test asks for it.
    RandomDice dice{ TEST_DICE_SEED };

    void SetUp() override {
        defaultEnhancement = ItemEnhancement{};
    }

    void TearDown() override {
        dice.set_test_mode(false);
        dice.clear_fixed_rolls();
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
    EXPECT_EQ(defaultEnhancement.damageBonus, 0);
    EXPECT_EQ(defaultEnhancement.toHitBonus, 0);
    EXPECT_EQ(defaultEnhancement.acBonus, 0);
    EXPECT_EQ(defaultEnhancement.strengthBonus, 0);
    EXPECT_EQ(defaultEnhancement.dexterityBonus, 0);
    EXPECT_EQ(defaultEnhancement.intelligenceBonus, 0);
    EXPECT_EQ(defaultEnhancement.hpBonus, 0);
    EXPECT_EQ(defaultEnhancement.manaBonus, 0);
    EXPECT_EQ(defaultEnhancement.speedBonus, 0);
    EXPECT_EQ(defaultEnhancement.stealthBonus, 0);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_ZeroResistances) {
    EXPECT_EQ(defaultEnhancement.fireResistance, 0);
    EXPECT_EQ(defaultEnhancement.coldResistance, 0);
    EXPECT_EQ(defaultEnhancement.lightningResistance, 0);
    EXPECT_EQ(defaultEnhancement.poisonResistance, 0);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_NotCursedOrBlessed) {
    EXPECT_EQ(defaultEnhancement.blessing, BlessingStatus::UNCURSED);
    EXPECT_FALSE(defaultEnhancement.isMagical);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_BaseValueModifier) {
    EXPECT_EQ(defaultEnhancement.valueModifier, 100);
}

TEST_F(EnhancementSystemTest, DefaultEnhancement_ZeroEnhancementLevel) {
    EXPECT_EQ(defaultEnhancement.enhancementLevel, 0);
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
// Apply Enhancement Effects Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, ApplyEffects_SharpPrefix_AddsDamageBonus) {
    defaultEnhancement.prefix = PrefixType::SHARP;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.damageBonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_KeenPrefix_AddsDamageBonus) {
    defaultEnhancement.prefix = PrefixType::KEEN;

    defaultEnhancement.apply_enhancement_effects();

    // Keen should give more damage than Sharp
    ItemEnhancement sharpEnhancement;
    sharpEnhancement.prefix = PrefixType::SHARP;
    sharpEnhancement.apply_enhancement_effects();

    EXPECT_GE(defaultEnhancement.damageBonus, sharpEnhancement.damageBonus);
}

TEST_F(EnhancementSystemTest, ApplyEffects_MasterworkPrefix_AddsToHit) {
    defaultEnhancement.prefix = PrefixType::MASTERWORK;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.toHitBonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_BlessedPrefix_AddsBothBonuses) {
    defaultEnhancement.prefix = PrefixType::BLESSED;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.damageBonus, 0);
    EXPECT_GT(defaultEnhancement.toHitBonus, 0);
    EXPECT_EQ(defaultEnhancement.blessing, BlessingStatus::BLESSED);
}

TEST_F(EnhancementSystemTest, ApplyEffects_ReinforcedPrefix_AddsACBonus) {
    defaultEnhancement.prefix = PrefixType::REINFORCED;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.acBonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_CursedPrefix_SetsCursedFlag) {
    defaultEnhancement.prefix = PrefixType::CURSED;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_EQ(defaultEnhancement.blessing, BlessingStatus::CURSED);
}

TEST_F(EnhancementSystemTest, ApplyEffects_OfSlayingSuffix_AddsDamage) {
    defaultEnhancement.suffix = SuffixType::OF_SLAYING;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.damageBonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_OfTheBearSuffix_AddsStrength) {
    defaultEnhancement.suffix = SuffixType::OF_THE_BEAR;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.strengthBonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_OfTheEagleSuffix_AddsDexterity) {
    defaultEnhancement.suffix = SuffixType::OF_THE_EAGLE;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.dexterityBonus, 0);
}

TEST_F(EnhancementSystemTest, ApplyEffects_OfFireResistance_AddsResistance) {
    defaultEnhancement.suffix = SuffixType::OF_FIRE_RESISTANCE;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GT(defaultEnhancement.fireResistance, 0);
}

// ----------------------------------------------------------------------------
// Resistance Value Range Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, FireResistance_InValidRange) {
    defaultEnhancement.suffix = SuffixType::OF_FIRE_RESISTANCE;
    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GE(defaultEnhancement.fireResistance, 0);
    EXPECT_LE(defaultEnhancement.fireResistance, 100);
}

TEST_F(EnhancementSystemTest, ColdResistance_InValidRange) {
    defaultEnhancement.suffix = SuffixType::OF_COLD_RESISTANCE;
    defaultEnhancement.apply_enhancement_effects();

    EXPECT_GE(defaultEnhancement.coldResistance, 0);
    EXPECT_LE(defaultEnhancement.coldResistance, 100);
}

// ----------------------------------------------------------------------------
// Random Generation Tests
// ----------------------------------------------------------------------------

// An enhancement is drawn from the game's own generator, so a test can queue the
// rolls and get a determined result. It used to draw from rand(), which no test
// could reach: the same scripted sequence gave a different answer every call.
//
// The weapon generator asks for two rolls - one in 0..99 deciding whether a
// prefix is drawn at all (under 40) and whether a suffix is (20 to 59), then one
// index into the nine-entry weapon prefix pool.
TEST_F(EnhancementSystemTest, ScriptedRollsDetermineTheDraw) {
    dice.set_test_mode(true);

    dice.set_next_roll(10);
    dice.set_next_roll(4);
    const PrefixType first = ItemEnhancement::generate_weapon_enhancement(MagicalPrefixes::ALLOWED, dice).prefix;

    dice.set_next_roll(10);
    dice.set_next_roll(4);
    const PrefixType again = ItemEnhancement::generate_weapon_enhancement(MagicalPrefixes::ALLOWED, dice).prefix;

    EXPECT_EQ(first, again) << "the same script gave two different draws, so the rolls are not what decides";

    // A different index has to reach a different prefix, or the first pair agreeing
    // would prove only that the pool has one entry.
    dice.set_next_roll(10);
    dice.set_next_roll(0);
    const PrefixType other = ItemEnhancement::generate_weapon_enhancement(MagicalPrefixes::ALLOWED, dice).prefix;

    EXPECT_NE(first, other) << "the index roll did not choose the prefix";
}

// The pool's order, pinned: index 4 of the weapon prefixes is FLAMING, and a roll
// of 10 draws a prefix and no suffix.
TEST_F(EnhancementSystemTest, TheIndexRollPicksOutOfTheWeaponPool) {
    dice.set_test_mode(true);
    dice.set_next_roll(10);
    dice.set_next_roll(4);

    const ItemEnhancement enhancement = ItemEnhancement::generate_weapon_enhancement(MagicalPrefixes::ALLOWED, dice);

    EXPECT_EQ(enhancement.prefix, PrefixType::FLAMING);
    EXPECT_EQ(enhancement.suffix, SuffixType::NONE) << "a roll of 10 is outside the suffix band";

}

// The span the index is drawn over, which a scripted roll cannot see: in test
// mode RandomDice hands back the queued value whatever range was asked for, so
// roll(0, size - 2) and roll(0, size - 1) both return an 8 that was queued. A
// seeded generator is asked instead, and the question is whether the pool's last
// entry is ever reached at all.
TEST_F(EnhancementSystemTest, EveryEntryOfTheWeaponPoolIsReachable) {
    constexpr int DRAWS = 400;
    constexpr unsigned int SEED = 20260928u;
    RandomDice seeded{ SEED };

    bool sawLastEntry = false;
    for (int draw = 0; draw < DRAWS && !sawLastEntry; ++draw) {
        sawLastEntry = ItemEnhancement::generate_weapon_enhancement(MagicalPrefixes::ALLOWED, seeded).prefix
            == PrefixType::CURSED;
    }

    EXPECT_TRUE(sawLastEntry) << "the last of the nine weapon prefixes is out of reach";
}

// The shop's ordinary equipment asks for no magical prefixes. Every pool holds
// some - FLAMING, FROST and SHOCK among the weapon prefixes, MAGICAL among the
// armour ones - so the choice has to reach whichever generator runs, and a draw
// that excludes them must never hand one back.
//
// Sampled rather than scripted: these generators draw from rand(), which nothing
// in the suite can queue. Unseeded it runs the same sequence every process, so
// the count below is the same on every run.
TEST_F(EnhancementSystemTest, ExcludingMagicalPrefixesExcludesThemFromEveryPool) {
    constexpr int DRAWS = 400;

    auto is_magical = [](PrefixType prefix) {
        return prefix == PrefixType::FLAMING
            || prefix == PrefixType::FROST
            || prefix == PrefixType::SHOCK
            || prefix == PrefixType::MAGICAL;
    };

    int magicalWeapons = 0;
    int magicalArmor = 0;
    for (int draw = 0; draw < DRAWS; ++draw) {
        if (is_magical(ItemEnhancement::generate_weapon_enhancement(MagicalPrefixes::EXCLUDED, dice).prefix)) {
            ++magicalWeapons;
        }
        if (is_magical(ItemEnhancement::generate_armor_enhancement(MagicalPrefixes::EXCLUDED, dice).prefix)) {
            ++magicalArmor;
        }
    }

    EXPECT_EQ(magicalWeapons, 0) << "a weapon drew FLAMING, FROST or SHOCK when the caller excluded them";
    EXPECT_EQ(magicalArmor, 0) << "armour drew MAGICAL when the caller excluded it";
}

// The counterpart, so the exclusion is shown to be doing something rather than
// the pools happening to hold nothing magical.
TEST_F(EnhancementSystemTest, AllowingMagicalPrefixesStillDrawsThem) {
    constexpr int DRAWS = 400;

    int magical = 0;
    for (int draw = 0; draw < DRAWS; ++draw) {
        const PrefixType prefix = ItemEnhancement::generate_weapon_enhancement(MagicalPrefixes::ALLOWED, dice).prefix;
        if (prefix == PrefixType::FLAMING || prefix == PrefixType::FROST || prefix == PrefixType::SHOCK) {
            ++magical;
        }
    }

    EXPECT_GT(magical, 0) << "the weapon pool never produced a magical prefix at all";
}

// The general generator, which the shop calls for anything neither weapon nor
// armour, rolls 0..99 once: under 30 a prefix, 30 to 54 a suffix, 55 to 59
// both, 60 and over nothing. A prefix then takes an index into the universal
// pool, a suffix one into the special-suffix pool, prefix first.
TEST_F(EnhancementSystemTest, TheBandRollDecidesWhatIsDrawn)
{
	struct BandCase
	{
		int bandRoll{ 0 };
		bool drawsPrefix{ false };
		bool drawsSuffix{ false };
	};

	const BandCase cases[] = {
		{ 29, true, false },
		{ 30, false, true },
		{ 54, false, true },
		{ 55, true, true },
		{ 59, true, true },
		{ 60, false, false },
	};

	for (const BandCase& bandCase : cases)
	{
		// Index 0 of each pool: BLESSED is not magical, so the exclusion cannot bear on the band.
		dice.set_next_roll(bandCase.bandRoll);
		if (bandCase.drawsPrefix)
		{
			dice.set_next_roll(0);
		}
		if (bandCase.drawsSuffix)
		{
			dice.set_next_roll(0);
		}

		const ItemEnhancement enhancement = ItemEnhancement::generate_random_enhancement(MagicalPrefixes::ALLOWED, dice);

		EXPECT_EQ(enhancement.prefix, bandCase.drawsPrefix ? PrefixType::BLESSED : PrefixType::NONE) << "band roll " << bandCase.bandRoll;
		EXPECT_EQ(enhancement.suffix, bandCase.drawsSuffix ? SuffixType::OF_SPEED : SuffixType::NONE) << "band roll " << bandCase.bandRoll;
	}
}

// In the both band the first index roll is the prefix's and the second the suffix's.
TEST_F(EnhancementSystemTest, TheBothBandDrawsThePrefixFirst)
{
	dice.set_next_roll(55);
	dice.set_next_roll(1);
	dice.set_next_roll(2);

	const ItemEnhancement enhancement = ItemEnhancement::generate_random_enhancement(MagicalPrefixes::ALLOWED, dice);

	EXPECT_EQ(enhancement.prefix, PrefixType::CURSED);
	EXPECT_EQ(enhancement.suffix, SuffixType::OF_MAGIC);
}

// The universal pool's order: index 0 is BLESSED, index 3 is MAGICAL.
TEST_F(EnhancementSystemTest, TheIndexRollPicksOutOfTheUniversalPool)
{
	dice.set_next_roll(10);
	dice.set_next_roll(0);
	const PrefixType first = ItemEnhancement::generate_random_enhancement(MagicalPrefixes::ALLOWED, dice).prefix;

	dice.set_next_roll(10);
	dice.set_next_roll(3);
	const PrefixType fourth = ItemEnhancement::generate_random_enhancement(MagicalPrefixes::ALLOWED, dice).prefix;

	EXPECT_EQ(first, PrefixType::BLESSED);
	EXPECT_EQ(fourth, PrefixType::MAGICAL);
}

// The special-suffix pool's order: index 0 is OF_SPEED.
TEST_F(EnhancementSystemTest, TheIndexRollPicksOutOfTheSpecialSuffixPool)
{
	dice.set_next_roll(40);
	dice.set_next_roll(0);

	const ItemEnhancement enhancement = ItemEnhancement::generate_random_enhancement(MagicalPrefixes::ALLOWED, dice);

	EXPECT_EQ(enhancement.prefix, PrefixType::NONE);
	EXPECT_EQ(enhancement.suffix, SuffixType::OF_SPEED);
}

// MAGICAL is the universal pool's one magical prefix; the same rolls keep it when
// allowed and lose it when the shop excludes magic.
TEST_F(EnhancementSystemTest, ExcludingMagicalPrefixesStripsMagicalFromTheGeneralPool)
{
	dice.set_next_roll(10);
	dice.set_next_roll(3);
	const PrefixType allowed = ItemEnhancement::generate_random_enhancement(MagicalPrefixes::ALLOWED, dice).prefix;

	dice.set_next_roll(10);
	dice.set_next_roll(3);
	const PrefixType excluded = ItemEnhancement::generate_random_enhancement(MagicalPrefixes::EXCLUDED, dice).prefix;

	EXPECT_EQ(allowed, PrefixType::MAGICAL);
	EXPECT_EQ(excluded, PrefixType::NONE);
}

// A range one short leaves a pool's last entry unreachable, which a scripted roll
// cannot see. Per draw CRACKED comes up with p = 0.35 / 6 and OF_BRITTLENESS with
// p = 0.30 / 8; over 1000 draws a correct pool misses them with (1 - p)^1000,
// 7.9e-27 and 2.5e-17.
TEST_F(EnhancementSystemTest, EveryEntryOfTheGeneralPoolsIsReachable)
{
	constexpr int DRAWS = 1000;
	constexpr unsigned int SEED = 20261019u;
	RandomDice seeded{ SEED };

	bool sawLastPrefix = false;
	bool sawLastSuffix = false;
	for (int draw = 0; draw < DRAWS; ++draw)
	{
		const ItemEnhancement enhancement = ItemEnhancement::generate_random_enhancement(MagicalPrefixes::ALLOWED, seeded);
		sawLastPrefix = sawLastPrefix || enhancement.prefix == PrefixType::CRACKED;
		sawLastSuffix = sawLastSuffix || enhancement.suffix == SuffixType::OF_BRITTLENESS;
	}

	EXPECT_TRUE(sawLastPrefix) << "CRACKED, the last universal prefix, is out of reach";
	EXPECT_TRUE(sawLastSuffix) << "OF_BRITTLENESS, the last special suffix, is out of reach";
}

// ----------------------------------------------------------------------------
// Value Modifier Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, ValueModifier_100_NoChange) {
    defaultEnhancement.valueModifier = 100;

    int baseValue = 100;
    int enhancedValue = (baseValue * defaultEnhancement.valueModifier) / 100;

    EXPECT_EQ(enhancedValue, 100);
}

TEST_F(EnhancementSystemTest, ValueModifier_150_IncreasesValue) {
    defaultEnhancement.valueModifier = 150;

    int baseValue = 100;
    int enhancedValue = (baseValue * defaultEnhancement.valueModifier) / 100;

    EXPECT_EQ(enhancedValue, 150);
}

TEST_F(EnhancementSystemTest, ValueModifier_50_DecreasesValue) {
    defaultEnhancement.valueModifier = 50;

    int baseValue = 100;
    int enhancedValue = (baseValue * defaultEnhancement.valueModifier) / 100;

    EXPECT_EQ(enhancedValue, 50);
}

// ----------------------------------------------------------------------------
// Combination Tests
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, PrefixAndSuffixBonuses_Stack) {
    defaultEnhancement.prefix = PrefixType::SHARP; // +damage
    defaultEnhancement.suffix = SuffixType::OF_SLAYING; // +damage

    defaultEnhancement.apply_enhancement_effects();

    // Both should contribute to damage
    EXPECT_GE(defaultEnhancement.damageBonus, 2);
}

TEST_F(EnhancementSystemTest, CursedItem_CanHaveOtherEnhancements) {
    defaultEnhancement.prefix = PrefixType::CURSED;
    defaultEnhancement.suffix = SuffixType::OF_SLAYING;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_EQ(defaultEnhancement.blessing, BlessingStatus::CURSED);
    // Still may have damage bonus from suffix
}

TEST_F(EnhancementSystemTest, BlessedItem_CannotBeCursed) {
    defaultEnhancement.prefix = PrefixType::BLESSED;

    defaultEnhancement.apply_enhancement_effects();

    EXPECT_EQ(defaultEnhancement.blessing, BlessingStatus::BLESSED);
    // Blessed should not be cursed
}

// ----------------------------------------------------------------------------
// Edge Cases
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, MultipleApplyEffects_Idempotent) {
    defaultEnhancement.prefix = PrefixType::SHARP;

    defaultEnhancement.apply_enhancement_effects();
    int firstDamageBonus = defaultEnhancement.damageBonus;

    defaultEnhancement.apply_enhancement_effects();
    int secondDamageBonus = defaultEnhancement.damageBonus;

    // Applying effects multiple times should be idempotent or additive
    // (depends on implementation - this documents behavior)
    EXPECT_GE(secondDamageBonus, firstDamageBonus);
}

// ----------------------------------------------------------------------------
// Integration with Item System
// ----------------------------------------------------------------------------

TEST_F(EnhancementSystemTest, WeaponDamageRegistry_AppliesEnhancement) {
    ItemEnhancement enhancement;
    enhancement.damageBonus = 3;

    DamageInfo baseDamage = WeaponDamageRegistry::get_damage_info("long_sword");
    DamageInfo enhancedDamage = WeaponDamageRegistry::get_enhanced_damage_info(
        "long_sword", &enhancement);

    EXPECT_EQ(enhancedDamage.minDamage, baseDamage.minDamage + 3);
    EXPECT_EQ(enhancedDamage.maxDamage, baseDamage.maxDamage + 3);
}

TEST_F(EnhancementSystemTest, WeaponDamageRegistry_NullEnhancement_ReturnsBase) {
    DamageInfo baseDamage = WeaponDamageRegistry::get_damage_info("dagger");
    DamageInfo withNull = WeaponDamageRegistry::get_enhanced_damage_info(
        "dagger", nullptr);

    EXPECT_EQ(withNull.minDamage, baseDamage.minDamage);
    EXPECT_EQ(withNull.maxDamage, baseDamage.maxDamage);
}
