// file: EditorFieldTest.cpp
// Every editor field's kind and label, and every plain number field's member,
// bounds, text and steps, against the code at 7630b235: the commit before the
// field descriptors.
//
// The expected rows below are generated, not typed. generate_field_tables.py
// read 7630b235's field_label switch (refusing any label that was not a string
// literal), field_is_string, field_is_toggle, SpellEditor's field_cycle and
// the TILE field for the kinds. It read field_adjust and field_value for the
// plain numbers: a case is plain when adjust is one clamped write and value
// prints that same direct member. So a row is what that commit's own code did,
// and a table that drifts from it fails here.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=EditorField*

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "src/EditorField.h"
#include "src/ItemEditor.h"
#include "src/ItemRegistry.h"
#include "src/MonsterEditor.h"
#include "src/MonsterRegistry.h"
#include "src/SpellEditor.h"

namespace
{

// One field as the old code had it: its position, enumerator, kind and label.
struct ExpectedField
{
	std::size_t index{ 0 };
	std::string_view name{};
	FieldKind kind{ FieldKind::TEXT };
	std::string_view label{};
};

// One plain number field as the old code had it: the member it read and the
// bounds it clamped to.
template <typename Record>
struct ExpectedPlain
{
	std::size_t index{ 0 };
	std::string_view name{};
	int Record::* member{ nullptr };
	int minimum{ 0 };
	int maximum{ 0 };
};

using ItemPlain = ExpectedPlain<ItemParams>;
using MonsterPlain = ExpectedPlain<MonsterParams>;

const ExpectedField ITEM_FIELDS_AT_HEAD[] = {
	ExpectedField{ 0, "NAME", FieldKind::TEXT, "Name" },
	ExpectedField{ 1, "APPEARANCE", FieldKind::TEXT, "Appearance" },
	ExpectedField{ 2, "CATEGORY", FieldKind::TEXT, "Category" },
	ExpectedField{ 3, "ITEM_CLASS", FieldKind::TOGGLE, "Item Class" },
	ExpectedField{ 4, "PICKABLE_TYPE", FieldKind::TOGGLE, "Pickable Type" },
	ExpectedField{ 5, "COLOR", FieldKind::TOGGLE, "Color" },
	ExpectedField{ 6, "VALUE", FieldKind::ADJUST, "Value" },
	ExpectedField{ 7, "BASE_WEIGHT", FieldKind::ADJUST, "Spawn Weight" },
	ExpectedField{ 8, "WEIGHT", FieldKind::ADJUST, "Carry Weight" },
	ExpectedField{ 9, "LEVEL_MIN", FieldKind::ADJUST, "Level Min" },
	ExpectedField{ 10, "LEVEL_MAX", FieldKind::ADJUST, "Level Max" },
	ExpectedField{ 11, "LEVEL_SCALING", FieldKind::ADJUST, "Level Scaling" },
	ExpectedField{
		12,
		"CONSUMABLE_EFFECT",
		FieldKind::TOGGLE,
		"Consumable Effect",
	},
	ExpectedField{
		13,
		"CONSUMABLE_BUFF",
		FieldKind::TOGGLE,
		"Consumable Buff",
	},
	ExpectedField{
		14,
		"CONSUMABLE_AMT_DICE",
		FieldKind::ADJUST,
		"Amount Dice",
	},
	ExpectedField{
		15,
		"CONSUMABLE_AMT_SIDES",
		FieldKind::ADJUST,
		"Amount Sides",
	},
	ExpectedField{
		16,
		"CONSUMABLE_AMT_BONUS",
		FieldKind::ADJUST,
		"Amount Bonus",
	},
	ExpectedField{ 17, "DURATION_DICE", FieldKind::ADJUST, "Duration Dice" },
	ExpectedField{ 18, "DURATION_SIDES", FieldKind::ADJUST, "Duration Sides" },
	ExpectedField{ 19, "DURATION_BONUS", FieldKind::ADJUST, "Duration Bonus" },
	ExpectedField{ 20, "DURATION_UNIT", FieldKind::TOGGLE, "Duration Unit" },
	ExpectedField{
		21,
		"AGES_YEARS",
		FieldKind::ADJUST,
		"Ages Drinker (years)",
	},
	ExpectedField{ 22, "TARGET_MODE", FieldKind::TOGGLE, "Target Mode" },
	ExpectedField{ 23, "SCROLL_ANIM", FieldKind::TOGGLE, "Scroll Animation" },
	ExpectedField{ 24, "RANGE", FieldKind::ADJUST, "Range" },
	ExpectedField{ 25, "DAMAGE", FieldKind::ADJUST, "Damage" },
	ExpectedField{ 26, "CONFUSE_TURNS", FieldKind::ADJUST, "Confuse Turns" },
	ExpectedField{ 27, "RANGED", FieldKind::TOGGLE, "Ranged" },
	ExpectedField{
		28,
		"HAND_REQUIREMENT",
		FieldKind::TOGGLE,
		"Hand Requirement",
	},
	ExpectedField{ 29, "WEAPON_SIZE", FieldKind::TOGGLE, "Weapon Size" },
	ExpectedField{ 30, "STRENGTH_RATING", FieldKind::ADJUST, "Made For STR" },
	ExpectedField{ 31, "AC_BONUS", FieldKind::ADJUST, "AC Bonus" },
	ExpectedField{ 32, "EFFECT", FieldKind::TOGGLE, "Effect" },
	ExpectedField{ 33, "EFFECT_BONUS", FieldKind::ADJUST, "Effect Bonus" },
	ExpectedField{ 34, "STR_BONUS", FieldKind::ADJUST, "STR Bonus" },
	ExpectedField{ 35, "DEX_BONUS", FieldKind::ADJUST, "DEX Bonus" },
	ExpectedField{ 36, "CON_BONUS", FieldKind::ADJUST, "CON Bonus" },
	ExpectedField{ 37, "INT_BONUS", FieldKind::ADJUST, "INT Bonus" },
	ExpectedField{ 38, "WIS_BONUS", FieldKind::ADJUST, "WIS Bonus" },
	ExpectedField{ 39, "CHA_BONUS", FieldKind::ADJUST, "CHA Bonus" },
	ExpectedField{ 40, "APPLICATION", FieldKind::TOGGLE, "Application" },
	ExpectedField{
		41,
		"EXCEPTIONAL_STRENGTH",
		FieldKind::ADJUST,
		"Exceptional STR",
	},
	ExpectedField{ 42, "NUTRITION", FieldKind::ADJUST, "Nutrition Value" },
	ExpectedField{ 43, "TILE", FieldKind::TILE, "Tile" },
};

const ExpectedField MONSTER_FIELDS_AT_HEAD[] = {
	ExpectedField{ 0, "NAME", FieldKind::TEXT, "Name" },
	ExpectedField{ 1, "CORPSE", FieldKind::TEXT, "Corpse" },
	ExpectedField{
		2,
		"CORPSE_NUTRITION",
		FieldKind::ADJUST,
		"Corpse Nutrition",
	},
	ExpectedField{ 3, "CORPSE_TASTE", FieldKind::TEXT, "Corpse Taste" },
	ExpectedField{ 4, "CORPSE_EDIBLE", FieldKind::TOGGLE, "Corpse Edible" },
	ExpectedField{ 5, "HD_LOWEST", FieldKind::ADJUST, "HD Lowest" },
	ExpectedField{ 6, "HD_HIGHEST", FieldKind::ADJUST, "HD Highest" },
	ExpectedField{ 7, "HD_SIDES", FieldKind::ADJUST, "HD Sides" },
	ExpectedField{ 8, "HD_BONUS", FieldKind::ADJUST, "HD Bonus" },
	ExpectedField{ 9, "THACO", FieldKind::ADJUST, "THAC0" },
	ExpectedField{ 10, "AC", FieldKind::ADJUST, "AC" },
	ExpectedField{ 11, "XP_COUNT", FieldKind::ADJUST, "XP For HD" },
	ExpectedField{ 12, "XP", FieldKind::ADJUST, "XP" },
	ExpectedField{ 13, "DR", FieldKind::ADJUST, "DR" },
	ExpectedField{ 14, "STR_SOURCE", FieldKind::TOGGLE, "Strength" },
	ExpectedField{ 15, "STR_SCORE", FieldKind::ADJUST, "STR Score" },
	ExpectedField{
		16,
		"STR_EXCEPTIONAL",
		FieldKind::ADJUST,
		"STR Exceptional",
	},
	ExpectedField{ 17, "WEAPON", FieldKind::TEXT, "Natural Attack" },
	ExpectedField{ 18, "DMG_NUM", FieldKind::ADJUST, "Dmg Num" },
	ExpectedField{ 19, "DMG_SIDES", FieldKind::ADJUST, "Dmg Sides" },
	ExpectedField{ 20, "DMG_BONUS", FieldKind::ADJUST, "Dmg Bonus" },
	ExpectedField{ 21, "DMG_TYPE", FieldKind::TOGGLE, "Dmg Type" },
	ExpectedField{ 22, "AI_TYPE", FieldKind::TOGGLE, "AI Type" },
	ExpectedField{ 23, "ETHICS", FieldKind::TOGGLE, "Ethics" },
	ExpectedField{ 24, "MORALITY", FieldKind::TOGGLE, "Morality" },
	ExpectedField{ 25, "SIZE", FieldKind::TOGGLE, "Size" },
	ExpectedField{ 26, "INTELLIGENCE", FieldKind::TOGGLE, "Intelligence" },
	ExpectedField{ 27, "CAN_SWIM", FieldKind::TOGGLE, "Can Swim" },
	ExpectedField{ 28, "CAN_FLY", FieldKind::TOGGLE, "Can Fly" },
	ExpectedField{ 29, "WALKS_WEBS", FieldKind::TOGGLE, "Walks Webs" },
	ExpectedField{ 30, "ATTITUDE", FieldKind::TOGGLE, "Attitude" },
	ExpectedField{ 31, "DISPLACEABLE", FieldKind::TOGGLE, "Displaceable" },
	ExpectedField{ 32, "CASTER_CLASS", FieldKind::TOGGLE, "Caster Class" },
	ExpectedField{ 33, "CASTER_LEVEL", FieldKind::ADJUST, "Caster Level" },
	ExpectedField{ 34, "INNATE_POWER", FieldKind::ADJUST, "Innate Power" },
	ExpectedField{ 35, "INNATE_SPELL", FieldKind::TEXT, "Power Spell" },
	ExpectedField{
		36,
		"INNATE_FREQUENCY",
		FieldKind::TOGGLE,
		"Power Frequency",
	},
	ExpectedField{ 37, "WEIGHT", FieldKind::ADJUST, "Spawn Weight" },
	ExpectedField{ 38, "DEPTH_MIN", FieldKind::ADJUST, "Depth Min" },
	ExpectedField{ 39, "DEPTH_MAX", FieldKind::ADJUST, "Depth Max" },
	ExpectedField{ 40, "TILE", FieldKind::TILE, "Tile" },
};

const ExpectedField SPELL_FIELDS_AT_HEAD[] = {
	ExpectedField{ 0, "NAME", FieldKind::TEXT, "Name" },
	ExpectedField{ 1, "LEVEL", FieldKind::ADJUST, "Level" },
	ExpectedField{ 2, "CLASS", FieldKind::TOGGLE, "Class" },
	ExpectedField{ 3, "EFFECT_TYPE", FieldKind::TOGGLE, "Effect" },
	ExpectedField{ 4, "DESCRIPTION", FieldKind::TEXT, "Description" },
};

const ItemPlain ITEM_PLAIN_AT_HEAD[] = {
	ItemPlain{ 6, "VALUE", &ItemParams::value, 0, 99999 },
	ItemPlain{ 7, "BASE_WEIGHT", &ItemParams::baseWeight, 0, 100 },
	ItemPlain{ 8, "WEIGHT", &ItemParams::weight, 0, 1000 },
	ItemPlain{ 9, "LEVEL_MIN", &ItemParams::levelMin, 1, 20 },
	ItemPlain{ 10, "LEVEL_MAX", &ItemParams::levelMax, 0, 20 },
	ItemPlain{ 21, "AGES_YEARS", &ItemParams::agesYears, -99, 99 },
	ItemPlain{ 24, "RANGE", &ItemParams::range, 0, 50 },
	ItemPlain{ 25, "DAMAGE", &ItemParams::damage, 0, 999 },
	ItemPlain{ 26, "CONFUSE_TURNS", &ItemParams::confuseTurns, 0, 999 },
	ItemPlain{ 30, "STRENGTH_RATING", &ItemParams::strengthRating, 0, 25 },
	ItemPlain{ 31, "AC_BONUS", &ItemParams::acBonus, -10, 10 },
	ItemPlain{ 33, "EFFECT_BONUS", &ItemParams::effectBonus, 0, 20 },
	ItemPlain{ 34, "STR_BONUS", &ItemParams::strBonus, -18, 18 },
	ItemPlain{ 35, "DEX_BONUS", &ItemParams::dexBonus, -18, 18 },
	ItemPlain{ 36, "CON_BONUS", &ItemParams::conBonus, -18, 18 },
	ItemPlain{ 37, "INT_BONUS", &ItemParams::intBonus, -18, 18 },
	ItemPlain{ 38, "WIS_BONUS", &ItemParams::wisBonus, -18, 18 },
	ItemPlain{ 39, "CHA_BONUS", &ItemParams::chaBonus, -18, 18 },
	ItemPlain{
		41,
		"EXCEPTIONAL_STRENGTH",
		&ItemParams::exceptionalStrength,
		0,
		100,
	},
	ItemPlain{ 42, "NUTRITION", &ItemParams::nutritionValue, 0, 9999 },
};

const MonsterPlain MONSTER_PLAIN_AT_HEAD[] = {
	MonsterPlain{
		2,
		"CORPSE_NUTRITION",
		&MonsterParams::corpseNutrition,
		0,
		9999,
	},
	MonsterPlain{ 9, "THACO", &MonsterParams::thaco, 1, 25 },
	MonsterPlain{ 10, "AC", &MonsterParams::ac, -10, 10 },
	MonsterPlain{ 13, "DR", &MonsterParams::dr, 0, 20 },
	MonsterPlain{ 37, "WEIGHT", &MonsterParams::baseWeight, 0, 100 },
	MonsterPlain{ 38, "DEPTH_MIN", &MonsterParams::levelMinimum, 1, 20 },
	MonsterPlain{ 39, "DEPTH_MAX", &MonsterParams::levelMaximum, 0, 20 },
};

// Every field's kind and label in one editor's table, compared with the old
// code's by position.
template <typename Table>
void expect_kinds_and_labels(
	std::string_view editor,
	const Table& table,
	std::span<const ExpectedField> fieldsAtHead)
{
	ASSERT_EQ(table.size(), fieldsAtHead.size()) << editor;
	for (const ExpectedField& expected : fieldsAtHead)
	{
		const std::string where = std::format("{} {}", editor, expected.name);
		EXPECT_EQ(table[expected.index].kind, expected.kind) << where;
		EXPECT_EQ(table[expected.index].label, expected.label) << where;
	}
}

// The old code's plain fields carry their member and bounds now, and only they
// do. Each one prints its member, steps one up and one down, and holds at
// either bound.
template <typename Record, typename Table>
void expect_plain_numbers(
	std::string_view editor,
	const Table& table,
	std::span<const ExpectedPlain<Record>> plainAtHead)
{
	std::size_t plainInTable = 0;
	for (const auto& descriptor : table)
	{
		plainInTable += descriptor.plain.has_value() ? 1 : 0;
	}
	EXPECT_EQ(plainInTable, plainAtHead.size()) << editor;

	for (const ExpectedPlain<Record>& expected : plainAtHead)
	{
		const std::string where = std::format("{} {}", editor, expected.name);
		const auto& plain = table[expected.index].plain;
		ASSERT_TRUE(plain.has_value()) << where << " lost its plain number";
		EXPECT_TRUE(plain->member == expected.member) << where;
		EXPECT_EQ(plain->minimum, expected.minimum) << where;
		EXPECT_EQ(plain->maximum, expected.maximum) << where;

		Record record{};
		int& value = record.*(expected.member);
		value = expected.minimum;
		const std::string leastText = std::to_string(expected.minimum);
		EXPECT_EQ(plain_text(*plain, record), leastText) << where;

		adjust_plain(*plain, record, 1);
		EXPECT_EQ(value, expected.minimum + 1) << where << " one step up";
		adjust_plain(*plain, record, -1);
		EXPECT_EQ(value, expected.minimum) << where << " one step down";
		adjust_plain(*plain, record, -1);
		EXPECT_EQ(value, expected.minimum) << where << " held at its least";

		value = expected.maximum;
		adjust_plain(*plain, record, 1);
		EXPECT_EQ(value, expected.maximum) << where << " held at its greatest";
	}
}

// A two-field table, small enough to break one rule at a time.
enum class SampleField
{
	FIRST,
	SECOND
};

struct SampleRecord
{
	int number{ 0 };
};

using SampleDescriptor = FieldDescriptor<SampleField, SampleRecord>;
using SamplePlain = PlainNumber<SampleRecord>;
using SampleTable = std::array<SampleDescriptor, 2>;

constexpr SamplePlain SAMPLE_PLAIN{ &SampleRecord::number, 0, 5 };
constexpr SamplePlain REVERSED_PLAIN{ &SampleRecord::number, 5, 0 };
constexpr SampleDescriptor SAMPLE_FIRST{
	SampleField::FIRST,
	FieldKind::ADJUST,
	"First",
	SAMPLE_PLAIN,
};
constexpr SampleDescriptor SAMPLE_SECOND{
	SampleField::SECOND,
	FieldKind::TEXT,
	"Second",
	std::nullopt,
};
constexpr SampleDescriptor TOGGLE_WITH_PLAIN{
	SampleField::FIRST,
	FieldKind::TOGGLE,
	"First",
	SAMPLE_PLAIN,
};
constexpr SampleDescriptor REVERSED_BOUNDS{
	SampleField::FIRST,
	FieldKind::ADJUST,
	"First",
	REVERSED_PLAIN,
};
constexpr SampleTable SAMPLE_TABLE{ { SAMPLE_FIRST, SAMPLE_SECOND } };
constexpr SampleTable SWAPPED{ { SAMPLE_SECOND, SAMPLE_FIRST } };
// The missing last entry is default-built, so it reads as FIRST.
constexpr SampleTable MISSING_LAST{ { SAMPLE_FIRST } };
constexpr SampleTable PLAIN_ON_TOGGLE{ { TOGGLE_WITH_PLAIN, SAMPLE_SECOND } };
constexpr SampleTable BOUNDS_REVERSED{ { REVERSED_BOUNDS, SAMPLE_SECOND } };

static_assert(is_well_formed_table(SAMPLE_TABLE));
static_assert(!is_well_formed_table(SWAPPED), "two entries swapped");
static_assert(!is_well_formed_table(MISSING_LAST), "the last entry missing");
static_assert(!is_well_formed_table(PLAIN_ON_TOGGLE), "plain on a toggle");
static_assert(!is_well_formed_table(BOUNDS_REVERSED), "bounds reversed");

} // namespace

TEST(EditorFieldTest, AFieldsDescriptorIsTheOneAtItsPosition)
{
	const auto& second = descriptor_of(SAMPLE_TABLE, SampleField::SECOND);
	EXPECT_EQ(second.label, "Second");
	const auto& first = descriptor_of(SAMPLE_TABLE, SampleField::FIRST);
	EXPECT_EQ(first.kind, FieldKind::ADJUST);
}

TEST(EditorFieldTest, TheItemEditorsFieldsAreTheOldCodesKindsAndLabels)
{
	expect_kinds_and_labels(
		"ItemEditor",
		ItemEditor::FIELDS,
		std::span{ ITEM_FIELDS_AT_HEAD });
}

TEST(EditorFieldTest, TheMonsterEditorsFieldsAreTheOldCodesKindsAndLabels)
{
	expect_kinds_and_labels(
		"MonsterEditor",
		MonsterEditor::FIELDS,
		std::span{ MONSTER_FIELDS_AT_HEAD });
}

TEST(EditorFieldTest, TheSpellEditorsFieldsAreTheOldCodesKindsAndLabels)
{
	expect_kinds_and_labels(
		"SpellEditor",
		SpellEditor::FIELDS,
		std::span{ SPELL_FIELDS_AT_HEAD });
}

TEST(EditorFieldTest, TheItemEditorsPlainNumbersShowAndStepAsTheOldCodeDid)
{
	expect_plain_numbers<ItemParams>(
		"ItemEditor",
		ItemEditor::FIELDS,
		std::span{ ITEM_PLAIN_AT_HEAD });
}

TEST(EditorFieldTest, TheMonsterEditorsPlainNumbersShowAndStepAsTheOldCodeDid)
{
	expect_plain_numbers<MonsterParams>(
		"MonsterEditor",
		MonsterEditor::FIELDS,
		std::span{ MONSTER_PLAIN_AT_HEAD });
}

// A field routed to a path its descriptor does not send it to stops the
// program and names it.
TEST(EditorFieldDeathTest, AMisroutedFieldAbortsNamingItsLabelAndThePath)
{
	EXPECT_DEATH(
		refuse_misrouted_field("Value", "ItemEditor::field_adjust"),
		"ItemEditor::field_adjust has no case for the field 'Value'");
}

// end of file: EditorFieldTest.cpp
