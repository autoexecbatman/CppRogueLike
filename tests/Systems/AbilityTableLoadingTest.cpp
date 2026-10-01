// file: AbilityTableLoadingTest.cpp
//
// What a row of a printed ability table has to carry, and what happens when it does
// not.
//
// The six tables in src/json are this project's transcription of the AD&D 2e printed
// tables, and the parser is the only schema they have. Every column each table prints
// is required: a row missing one is a broken transcription, not a row with a zero in
// it. Until 2026-10-01 the loaders read thirty-two of those columns with j.value()
// and a default, so a dropped column became a zero and the character sheet it fed
// said nothing. That is the same failure CLAUDE.md records for items.json, where
// every field silently defaulted and create_random_of_category returned nullptr
// forever with no error to find.
//
// The rows below are copied out of src/json, so the happy-path expectations are the
// table's own numbers rather than whatever the parser currently returns.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=AbilityTableLoadingTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "src/DataManager.h"

using json = nlohmann::json;

namespace
{
// Strength 17, straight out of src/json/strength.json.
json strength_seventeen()
{
	return json{
		{ "Str", 17 }, { "Hit", 1 }, { "Dmg", 1 }, { "Wgt", 85 }, { "MaxPress", 220 }, { "maxCarried", 220 }, { "OpenDoors", 10 }, { "BB_LG", 13 }, { "Notes", "" }, { "encumbrance", { { "unencumberedTo", 85 }, { "lightTo", 121 }, { "moderateTo", 157 }, { "heavyTo", 193 } } }
	};
}

json dexterity_seventeen()
{
	return json{ { "Dex", 17 }, { "ReactionAdj", 2 }, { "MissileAttackAdj", 2 }, { "DefensiveAdj", -3 } };
}

json constitution_seventeen()
{
	return json{
		{ "Con", 17 }, { "HPAdj", 3 }, { "hitDieMinimum", 1 }, { "SystemShock", 97 }, { "ResurrectionSurvival", 98 }, { "PoisonSave", 0 }, { "Regeneration", 0 }
	};
}

json charisma_seventeen()
{
	return json{ { "Cha", 17 }, { "MaxHencmen", 10 }, { "Loyalty", 6 }, { "ReactionAdj", 6 } };
}

json intelligence_seventeen()
{
	return json{
		{ "Int", 17 }, { "NumberOfLanguages", 6 }, { "SpellLevel", 8 }, { "ChanceToLearnSpell", 75 }, { "MaxNumberOfSpells", 14 }, { "IllusionImmunity", 0 }
	};
}

json wisdom_seventeen()
{
	return json{
		{ "Wis", 17 }, { "MagicalDefenseAdj", 3 }, { "ChanceOfSpellFailure", 0 }, { "SpellImmunity", 0 }, { "bonusSpells", json::array({ 3 }) }
	};
}
} // namespace

// The happy path, one per table: a complete row comes back as the table prints it.
TEST(AbilityTableLoadingTest, ACompleteStrengthRowParses)
{
	const StrengthAttributes row = strength_row_from(strength_seventeen());

	EXPECT_EQ(row.Str, 17);
	EXPECT_EQ(row.hitProb, 1);
	EXPECT_EQ(row.dmgAdj, 1);
	EXPECT_EQ(row.maxPress, 220);
	EXPECT_EQ(row.openDoors, 10);
	// A plain score carries no percentile band.
	EXPECT_EQ(row.exceptionalFrom, 0);
}

TEST(AbilityTableLoadingTest, ACompleteDexterityRowParses)
{
	const DexterityAttributes row = dexterity_row_from(dexterity_seventeen());

	EXPECT_EQ(row.Dex, 17);
	EXPECT_EQ(row.ReactionAdj, 2);
	EXPECT_EQ(row.MissileAttackAdj, 2);
	EXPECT_EQ(row.DefensiveAdj, -3);
}

TEST(AbilityTableLoadingTest, ACompleteConstitutionRowParses)
{
	const ConstitutionAttributes row = constitution_row_from(constitution_seventeen());

	EXPECT_EQ(row.Con, 17);
	EXPECT_EQ(row.HPAdj, 3);
	EXPECT_EQ(row.SystemShock, 97);
	EXPECT_EQ(row.ResurrectionSurvival, 98);
}

TEST(AbilityTableLoadingTest, ACompleteCharismaRowParses)
{
	const CharismaAttributes row = charisma_row_from(charisma_seventeen());

	EXPECT_EQ(row.Cha, 17);
	EXPECT_EQ(row.MaxHencmen, 10);
	EXPECT_EQ(row.Loyalty, 6);
	EXPECT_EQ(row.ReactionAdj, 6);
}

TEST(AbilityTableLoadingTest, ACompleteIntelligenceRowParses)
{
	const IntelligenceAttributes row = intelligence_row_from(intelligence_seventeen());

	EXPECT_EQ(row.Int, 17);
	EXPECT_EQ(row.NumberOfLanguages, 6);
	EXPECT_EQ(row.ChanceToLearnSpell, 75);
	EXPECT_EQ(row.MaxNumberOfSpells, 14);
}

TEST(AbilityTableLoadingTest, ACompleteWisdomRowParses)
{
	const WisdomAttributes row = wisdom_row_from(wisdom_seventeen());

	EXPECT_EQ(row.Wis, 17);
	EXPECT_EQ(row.MagicalDefenseAdj, 3);
	ASSERT_EQ(row.bonusSpells.size(), 1u);
	EXPECT_EQ(row.bonusSpells.at(0), 3);
}

// And the half that was missing: a dropped column is a broken table and says so.
// Each of these silently produced a zero before 2026-10-01.
TEST(AbilityTableLoadingTest, AStrengthRowMissingItsHitBonusIsRefused)
{
	json row = strength_seventeen();
	row.erase("Hit");

	EXPECT_THROW([[maybe_unused]] const auto parsed = strength_row_from(row), nlohmann::json::out_of_range)
		<< "a Strength row with no hit bonus loaded as a zero instead of being refused";
}

TEST(AbilityTableLoadingTest, ADexterityRowMissingItsDefensiveAdjustmentIsRefused)
{
	json row = dexterity_seventeen();
	row.erase("DefensiveAdj");

	EXPECT_THROW([[maybe_unused]] const auto parsed = dexterity_row_from(row), nlohmann::json::out_of_range)
		<< "a Dexterity row with no defensive adjustment loaded as a zero, which reads as armour class 0";
}

TEST(AbilityTableLoadingTest, AConstitutionRowMissingItsHitPointAdjustmentIsRefused)
{
	json row = constitution_seventeen();
	row.erase("HPAdj");

	EXPECT_THROW([[maybe_unused]] const auto parsed = constitution_row_from(row), nlohmann::json::out_of_range)
		<< "a Constitution row with no hit point adjustment loaded as a zero";
}

TEST(AbilityTableLoadingTest, ACharismaRowMissingItsLoyaltyIsRefused)
{
	json row = charisma_seventeen();
	row.erase("Loyalty");

	EXPECT_THROW([[maybe_unused]] const auto parsed = charisma_row_from(row), nlohmann::json::out_of_range)
		<< "a Charisma row with no loyalty base loaded as a zero";
}

TEST(AbilityTableLoadingTest, AnIntelligenceRowMissingItsSpellLearningChanceIsRefused)
{
	json row = intelligence_seventeen();
	row.erase("ChanceToLearnSpell");

	EXPECT_THROW([[maybe_unused]] const auto parsed = intelligence_row_from(row), nlohmann::json::out_of_range)
		<< "an Intelligence row with no chance to learn a spell loaded as a zero, so every spell fails";
}

TEST(AbilityTableLoadingTest, AWisdomRowMissingItsSpellFailureChanceIsRefused)
{
	json row = wisdom_seventeen();
	row.erase("ChanceOfSpellFailure");

	EXPECT_THROW([[maybe_unused]] const auto parsed = wisdom_row_from(row), nlohmann::json::out_of_range)
		<< "a Wisdom row with no spell failure chance loaded as a zero, which is the best possible roll";
}

// The one column that is genuinely optional, and stays that way: only the 18/xx band
// rows carry a percentile range, so a plain score's row must still parse without one.
TEST(AbilityTableLoadingTest, APlainStrengthRowWithNoPercentileBandStillParses)
{
	const StrengthAttributes row = strength_row_from(strength_seventeen());

	EXPECT_EQ(row.exceptionalFrom, 0);
	EXPECT_EQ(row.exceptionalTo, 0);
}
