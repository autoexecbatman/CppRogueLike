// file: EditorFieldTest.cpp
// Every editor field's kind and label, and every plain number field's member, bounds, text and
// steps, against the code at 7630b235: the commit before the field descriptors.
//
// The expected rows below are generated, not typed. generate_field_tables.py read 7630b235's
// field_label switch (refusing any label that was not a string literal), field_is_string,
// field_is_toggle, SpellEditor's field_cycle and the TILE field for the kinds. It read
// field_adjust and field_value for the plain numbers: a case is plain when adjust is one clamped
// write and value prints that same direct member. So a row is what that commit's own code did,
// and a table that drifts from it fails here.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=EditorFieldTest.*

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
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

// One field as the old code had it: its editor, its position, its enumerator, its kind and label.
struct ExpectedField
{
	std::string_view editor{};
	std::size_t index{ 0 };
	std::string_view name{};
	FieldKind kind{ FieldKind::TEXT };
	std::string_view label{};
};

// One plain number field as the old code had it: the member it read and the bounds it clamped to.
template <typename Record>
struct ExpectedPlain
{
	std::size_t index{ 0 };
	std::string_view name{};
	int Record::* member{ nullptr };
	int minimum{ 0 };
	int maximum{ 0 };
};

// Generated from 7630b235 by generate_field_tables.py; every row is that commit's own code.
constexpr ExpectedField FIELDS_AT_HEAD[] = {
	ExpectedField{ "ItemEditor", 0, "NAME", FieldKind::TEXT, "Name" },
	ExpectedField{ "ItemEditor", 1, "CATEGORY", FieldKind::TEXT, "Category" },
	ExpectedField{ "ItemEditor", 2, "ITEM_CLASS", FieldKind::TOGGLE, "Item Class" },
	ExpectedField{ "ItemEditor", 3, "PICKABLE_TYPE", FieldKind::TOGGLE, "Pickable Type" },
	ExpectedField{ "ItemEditor", 4, "COLOR", FieldKind::TOGGLE, "Color" },
	ExpectedField{ "ItemEditor", 5, "VALUE", FieldKind::ADJUST, "Value" },
	ExpectedField{ "ItemEditor", 6, "BASE_WEIGHT", FieldKind::ADJUST, "Spawn Weight" },
	ExpectedField{ "ItemEditor", 7, "WEIGHT", FieldKind::ADJUST, "Carry Weight" },
	ExpectedField{ "ItemEditor", 8, "LEVEL_MIN", FieldKind::ADJUST, "Level Min" },
	ExpectedField{ "ItemEditor", 9, "LEVEL_MAX", FieldKind::ADJUST, "Level Max" },
	ExpectedField{ "ItemEditor", 10, "LEVEL_SCALING", FieldKind::ADJUST, "Level Scaling" },
	ExpectedField{ "ItemEditor", 11, "CONSUMABLE_EFFECT", FieldKind::TOGGLE, "Consumable Effect" },
	ExpectedField{ "ItemEditor", 12, "CONSUMABLE_BUFF", FieldKind::TOGGLE, "Consumable Buff" },
	ExpectedField{ "ItemEditor", 13, "CONSUMABLE_AMT_DICE", FieldKind::ADJUST, "Amount Dice" },
	ExpectedField{ "ItemEditor", 14, "CONSUMABLE_AMT_SIDES", FieldKind::ADJUST, "Amount Sides" },
	ExpectedField{ "ItemEditor", 15, "CONSUMABLE_AMT_BONUS", FieldKind::ADJUST, "Amount Bonus" },
	ExpectedField{ "ItemEditor", 16, "DURATION_DICE", FieldKind::ADJUST, "Duration Dice" },
	ExpectedField{ "ItemEditor", 17, "DURATION_SIDES", FieldKind::ADJUST, "Duration Sides" },
	ExpectedField{ "ItemEditor", 18, "DURATION_BONUS", FieldKind::ADJUST, "Duration Bonus" },
	ExpectedField{ "ItemEditor", 19, "DURATION_UNIT", FieldKind::TOGGLE, "Duration Unit" },
	ExpectedField{ "ItemEditor", 20, "AGES_YEARS", FieldKind::ADJUST, "Ages Drinker (years)" },
	ExpectedField{ "ItemEditor", 21, "TARGET_MODE", FieldKind::TOGGLE, "Target Mode" },
	ExpectedField{ "ItemEditor", 22, "SCROLL_ANIM", FieldKind::TOGGLE, "Scroll Animation" },
	ExpectedField{ "ItemEditor", 23, "RANGE", FieldKind::ADJUST, "Range" },
	ExpectedField{ "ItemEditor", 24, "DAMAGE", FieldKind::ADJUST, "Damage" },
	ExpectedField{ "ItemEditor", 25, "CONFUSE_TURNS", FieldKind::ADJUST, "Confuse Turns" },
	ExpectedField{ "ItemEditor", 26, "RANGED", FieldKind::TOGGLE, "Ranged" },
	ExpectedField{ "ItemEditor", 27, "HAND_REQUIREMENT", FieldKind::TOGGLE, "Hand Requirement" },
	ExpectedField{ "ItemEditor", 28, "WEAPON_SIZE", FieldKind::TOGGLE, "Weapon Size" },
	ExpectedField{ "ItemEditor", 29, "STRENGTH_RATING", FieldKind::ADJUST, "Made For STR" },
	ExpectedField{ "ItemEditor", 30, "AC_BONUS", FieldKind::ADJUST, "AC Bonus" },
	ExpectedField{ "ItemEditor", 31, "EFFECT", FieldKind::TOGGLE, "Effect" },
	ExpectedField{ "ItemEditor", 32, "EFFECT_BONUS", FieldKind::ADJUST, "Effect Bonus" },
	ExpectedField{ "ItemEditor", 33, "STR_BONUS", FieldKind::ADJUST, "STR Bonus" },
	ExpectedField{ "ItemEditor", 34, "DEX_BONUS", FieldKind::ADJUST, "DEX Bonus" },
	ExpectedField{ "ItemEditor", 35, "CON_BONUS", FieldKind::ADJUST, "CON Bonus" },
	ExpectedField{ "ItemEditor", 36, "INT_BONUS", FieldKind::ADJUST, "INT Bonus" },
	ExpectedField{ "ItemEditor", 37, "WIS_BONUS", FieldKind::ADJUST, "WIS Bonus" },
	ExpectedField{ "ItemEditor", 38, "CHA_BONUS", FieldKind::ADJUST, "CHA Bonus" },
	ExpectedField{ "ItemEditor", 39, "APPLICATION", FieldKind::TOGGLE, "Application" },
	ExpectedField{ "ItemEditor", 40, "EXCEPTIONAL_STRENGTH", FieldKind::ADJUST, "Exceptional STR" },
	ExpectedField{ "ItemEditor", 41, "NUTRITION", FieldKind::ADJUST, "Nutrition Value" },
	ExpectedField{ "ItemEditor", 42, "TILE", FieldKind::TILE, "Tile" },
	ExpectedField{ "MonsterEditor", 0, "NAME", FieldKind::TEXT, "Name" },
	ExpectedField{ "MonsterEditor", 1, "CORPSE", FieldKind::TEXT, "Corpse" },
	ExpectedField{ "MonsterEditor", 2, "CORPSE_NUTRITION", FieldKind::ADJUST, "Corpse Nutrition" },
	ExpectedField{ "MonsterEditor", 3, "CORPSE_TASTE", FieldKind::TEXT, "Corpse Taste" },
	ExpectedField{ "MonsterEditor", 4, "CORPSE_EDIBLE", FieldKind::TOGGLE, "Corpse Edible" },
	ExpectedField{ "MonsterEditor", 5, "HD_LOWEST", FieldKind::ADJUST, "HD Lowest" },
	ExpectedField{ "MonsterEditor", 6, "HD_HIGHEST", FieldKind::ADJUST, "HD Highest" },
	ExpectedField{ "MonsterEditor", 7, "HD_SIDES", FieldKind::ADJUST, "HD Sides" },
	ExpectedField{ "MonsterEditor", 8, "HD_BONUS", FieldKind::ADJUST, "HD Bonus" },
	ExpectedField{ "MonsterEditor", 9, "THACO", FieldKind::ADJUST, "THAC0" },
	ExpectedField{ "MonsterEditor", 10, "AC", FieldKind::ADJUST, "AC" },
	ExpectedField{ "MonsterEditor", 11, "XP_COUNT", FieldKind::ADJUST, "XP For HD" },
	ExpectedField{ "MonsterEditor", 12, "XP", FieldKind::ADJUST, "XP" },
	ExpectedField{ "MonsterEditor", 13, "DR", FieldKind::ADJUST, "DR" },
	ExpectedField{ "MonsterEditor", 14, "STR_SOURCE", FieldKind::TOGGLE, "Strength" },
	ExpectedField{ "MonsterEditor", 15, "STR_SCORE", FieldKind::ADJUST, "STR Score" },
	ExpectedField{ "MonsterEditor", 16, "STR_EXCEPTIONAL", FieldKind::ADJUST, "STR Exceptional" },
	ExpectedField{ "MonsterEditor", 17, "WEAPON", FieldKind::TEXT, "Natural Attack" },
	ExpectedField{ "MonsterEditor", 18, "DMG_NUM", FieldKind::ADJUST, "Dmg Num" },
	ExpectedField{ "MonsterEditor", 19, "DMG_SIDES", FieldKind::ADJUST, "Dmg Sides" },
	ExpectedField{ "MonsterEditor", 20, "DMG_BONUS", FieldKind::ADJUST, "Dmg Bonus" },
	ExpectedField{ "MonsterEditor", 21, "DMG_TYPE", FieldKind::TOGGLE, "Dmg Type" },
	ExpectedField{ "MonsterEditor", 22, "AI_TYPE", FieldKind::TOGGLE, "AI Type" },
	ExpectedField{ "MonsterEditor", 23, "ETHICS", FieldKind::TOGGLE, "Ethics" },
	ExpectedField{ "MonsterEditor", 24, "MORALITY", FieldKind::TOGGLE, "Morality" },
	ExpectedField{ "MonsterEditor", 25, "SIZE", FieldKind::TOGGLE, "Size" },
	ExpectedField{ "MonsterEditor", 26, "INTELLIGENCE", FieldKind::TOGGLE, "Intelligence" },
	ExpectedField{ "MonsterEditor", 27, "CAN_SWIM", FieldKind::TOGGLE, "Can Swim" },
	ExpectedField{ "MonsterEditor", 28, "CAN_FLY", FieldKind::TOGGLE, "Can Fly" },
	ExpectedField{ "MonsterEditor", 29, "WALKS_WEBS", FieldKind::TOGGLE, "Walks Webs" },
	ExpectedField{ "MonsterEditor", 30, "ATTITUDE", FieldKind::TOGGLE, "Attitude" },
	ExpectedField{ "MonsterEditor", 31, "DISPLACEABLE", FieldKind::TOGGLE, "Displaceable" },
	ExpectedField{ "MonsterEditor", 32, "CASTER_CLASS", FieldKind::TOGGLE, "Caster Class" },
	ExpectedField{ "MonsterEditor", 33, "CASTER_LEVEL", FieldKind::ADJUST, "Caster Level" },
	ExpectedField{ "MonsterEditor", 34, "INNATE_POWER", FieldKind::ADJUST, "Innate Power" },
	ExpectedField{ "MonsterEditor", 35, "INNATE_SPELL", FieldKind::TEXT, "Power Spell" },
	ExpectedField{ "MonsterEditor", 36, "INNATE_FREQUENCY", FieldKind::TOGGLE, "Power Frequency" },
	ExpectedField{ "MonsterEditor", 37, "WEIGHT", FieldKind::ADJUST, "Spawn Weight" },
	ExpectedField{ "MonsterEditor", 38, "DEPTH_MIN", FieldKind::ADJUST, "Depth Min" },
	ExpectedField{ "MonsterEditor", 39, "DEPTH_MAX", FieldKind::ADJUST, "Depth Max" },
	ExpectedField{ "MonsterEditor", 40, "TILE", FieldKind::TILE, "Tile" },
	ExpectedField{ "SpellEditor", 0, "NAME", FieldKind::TEXT, "Name" },
	ExpectedField{ "SpellEditor", 1, "LEVEL", FieldKind::ADJUST, "Level" },
	ExpectedField{ "SpellEditor", 2, "CLASS", FieldKind::TOGGLE, "Class" },
	ExpectedField{ "SpellEditor", 3, "EFFECT_TYPE", FieldKind::TOGGLE, "Effect" },
	ExpectedField{ "SpellEditor", 4, "DESCRIPTION", FieldKind::TEXT, "Description" },
};

const ExpectedPlain<ItemParams> ITEMEDITOR_PLAIN_AT_HEAD[] = {
	ExpectedPlain<ItemParams>{ 5, "VALUE", &ItemParams::value, 0, 99999 },
	ExpectedPlain<ItemParams>{ 6, "BASE_WEIGHT", &ItemParams::baseWeight, 0, 100 },
	ExpectedPlain<ItemParams>{ 7, "WEIGHT", &ItemParams::weight, 0, 1000 },
	ExpectedPlain<ItemParams>{ 8, "LEVEL_MIN", &ItemParams::levelMin, 1, 20 },
	ExpectedPlain<ItemParams>{ 9, "LEVEL_MAX", &ItemParams::levelMax, 0, 20 },
	ExpectedPlain<ItemParams>{ 20, "AGES_YEARS", &ItemParams::agesYears, -99, 99 },
	ExpectedPlain<ItemParams>{ 23, "RANGE", &ItemParams::range, 0, 50 },
	ExpectedPlain<ItemParams>{ 24, "DAMAGE", &ItemParams::damage, 0, 999 },
	ExpectedPlain<ItemParams>{ 25, "CONFUSE_TURNS", &ItemParams::confuseTurns, 0, 999 },
	ExpectedPlain<ItemParams>{ 29, "STRENGTH_RATING", &ItemParams::strengthRating, 0, 25 },
	ExpectedPlain<ItemParams>{ 30, "AC_BONUS", &ItemParams::acBonus, -10, 10 },
	ExpectedPlain<ItemParams>{ 32, "EFFECT_BONUS", &ItemParams::effectBonus, 0, 20 },
	ExpectedPlain<ItemParams>{ 33, "STR_BONUS", &ItemParams::strBonus, -18, 18 },
	ExpectedPlain<ItemParams>{ 34, "DEX_BONUS", &ItemParams::dexBonus, -18, 18 },
	ExpectedPlain<ItemParams>{ 35, "CON_BONUS", &ItemParams::conBonus, -18, 18 },
	ExpectedPlain<ItemParams>{ 36, "INT_BONUS", &ItemParams::intBonus, -18, 18 },
	ExpectedPlain<ItemParams>{ 37, "WIS_BONUS", &ItemParams::wisBonus, -18, 18 },
	ExpectedPlain<ItemParams>{ 38, "CHA_BONUS", &ItemParams::chaBonus, -18, 18 },
	ExpectedPlain<ItemParams>{ 40, "EXCEPTIONAL_STRENGTH", &ItemParams::exceptionalStrength, 0, 100 },
	ExpectedPlain<ItemParams>{ 41, "NUTRITION", &ItemParams::nutritionValue, 0, 9999 },
};

const ExpectedPlain<MonsterParams> MONSTEREDITOR_PLAIN_AT_HEAD[] = {
	ExpectedPlain<MonsterParams>{ 2, "CORPSE_NUTRITION", &MonsterParams::corpseNutrition, 0, 9999 },
	ExpectedPlain<MonsterParams>{ 9, "THACO", &MonsterParams::thaco, 1, 25 },
	ExpectedPlain<MonsterParams>{ 10, "AC", &MonsterParams::ac, -10, 10 },
	ExpectedPlain<MonsterParams>{ 13, "DR", &MonsterParams::dr, 0, 20 },
	ExpectedPlain<MonsterParams>{ 37, "WEIGHT", &MonsterParams::baseWeight, 0, 100 },
	ExpectedPlain<MonsterParams>{ 38, "DEPTH_MIN", &MonsterParams::levelMinimum, 1, 20 },
	ExpectedPlain<MonsterParams>{ 39, "DEPTH_MAX", &MonsterParams::levelMaximum, 0, 20 },
};

// Every field's kind and label in one editor's table, compared with the old code's by position.
template <typename Table>
void expect_kinds_and_labels(std::string_view editor, const Table& table)
{
	std::size_t seen = 0;
	for (const ExpectedField& expected : FIELDS_AT_HEAD)
	{
		if (expected.editor != editor)
		{
			continue;
		}
		++seen;
		ASSERT_LT(expected.index, table.size()) << editor << " lost " << expected.name;
		EXPECT_EQ(table[expected.index].kind, expected.kind) << editor << " " << expected.name;
		EXPECT_EQ(table[expected.index].label, expected.label) << editor << " " << expected.name;
	}
	EXPECT_EQ(seen, table.size()) << editor << " has a field the old code did not";
}

// The old code's plain fields carry their member and bounds now, and only they do. Each one
// prints its member, steps one up and one down, and holds at either bound.
template <typename Record, typename Table>
void expect_plain_numbers(std::string_view editor, const Table& table, std::span<const ExpectedPlain<Record>> plainAtHead)
{
	std::size_t plainInTable = 0;
	for (const auto& descriptor : table)
	{
		plainInTable += descriptor.plain.has_value() ? 1 : 0;
	}
	EXPECT_EQ(plainInTable, plainAtHead.size()) << editor << " marks a different set of fields plain";

	for (const ExpectedPlain<Record>& expected : plainAtHead)
	{
		const auto& plain = table[expected.index].plain;
		ASSERT_TRUE(plain.has_value()) << editor << " " << expected.name << " lost its plain number";
		EXPECT_TRUE(plain->member == expected.member) << editor << " " << expected.name << " reads another member";
		EXPECT_EQ(plain->minimum, expected.minimum) << editor << " " << expected.name;
		EXPECT_EQ(plain->maximum, expected.maximum) << editor << " " << expected.name;

		Record record{};
		record.*(expected.member) = expected.minimum;
		EXPECT_EQ(plain_text(*plain, record), std::to_string(expected.minimum)) << editor << " " << expected.name;

		adjust_plain(*plain, record, 1);
		EXPECT_EQ(record.*(expected.member), expected.minimum + 1) << editor << " " << expected.name << " one step up";
		adjust_plain(*plain, record, -1);
		EXPECT_EQ(record.*(expected.member), expected.minimum) << editor << " " << expected.name << " one step down";
		adjust_plain(*plain, record, -1);
		EXPECT_EQ(record.*(expected.member), expected.minimum) << editor << " " << expected.name << " held at its least";

		record.*(expected.member) = expected.maximum;
		adjust_plain(*plain, record, 1);
		EXPECT_EQ(record.*(expected.member), expected.maximum) << editor << " " << expected.name << " held at its greatest";
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
using SampleTable = std::array<SampleDescriptor, 2>;

constexpr PlainNumber<SampleRecord> SAMPLE_PLAIN{ &SampleRecord::number, 0, 5 };
constexpr SampleDescriptor SAMPLE_FIRST{ SampleField::FIRST, FieldKind::ADJUST, "First", SAMPLE_PLAIN };
constexpr SampleDescriptor SAMPLE_SECOND{ SampleField::SECOND, FieldKind::TEXT, "Second", std::nullopt };
constexpr SampleTable SAMPLE_TABLE{ { SAMPLE_FIRST, SAMPLE_SECOND } };

static_assert(is_well_formed_table(SAMPLE_TABLE));
static_assert(!is_well_formed_table(SampleTable{ { SAMPLE_SECOND, SAMPLE_FIRST } }), "two entries swapped");
static_assert(!is_well_formed_table(SampleTable{ { SAMPLE_FIRST } }), "the last entry missing, so default-built as FIRST");
static_assert(!is_well_formed_table(SampleTable{ { SampleDescriptor{ SampleField::FIRST, FieldKind::TOGGLE, "First", SAMPLE_PLAIN }, SAMPLE_SECOND } }), "a plain number on a toggle");
static_assert(!is_well_formed_table(SampleTable{ { SampleDescriptor{ SampleField::FIRST, FieldKind::ADJUST, "First", PlainNumber<SampleRecord>{ &SampleRecord::number, 5, 0 } }, SAMPLE_SECOND } }), "bounds reversed");

} // namespace

TEST(EditorFieldTest, AFieldsDescriptorIsTheOneAtItsPosition)
{
	EXPECT_EQ(descriptor_of(SAMPLE_TABLE, SampleField::SECOND).label, "Second");
	EXPECT_EQ(descriptor_of(SAMPLE_TABLE, SampleField::FIRST).kind, FieldKind::ADJUST);
}

TEST(EditorFieldTest, TheItemEditorsFieldsAreTheOldCodesKindsAndLabels)
{
	expect_kinds_and_labels("ItemEditor", ItemEditor::FIELDS);
}

TEST(EditorFieldTest, TheMonsterEditorsFieldsAreTheOldCodesKindsAndLabels)
{
	expect_kinds_and_labels("MonsterEditor", MonsterEditor::FIELDS);
}

TEST(EditorFieldTest, TheSpellEditorsFieldsAreTheOldCodesKindsAndLabels)
{
	expect_kinds_and_labels("SpellEditor", SpellEditor::FIELDS);
}

TEST(EditorFieldTest, TheItemEditorsPlainNumbersShowAndStepAsTheOldCodeDid)
{
	expect_plain_numbers<ItemParams>("ItemEditor", ItemEditor::FIELDS, std::span{ ITEMEDITOR_PLAIN_AT_HEAD });
}

TEST(EditorFieldTest, TheMonsterEditorsPlainNumbersShowAndStepAsTheOldCodeDid)
{
	expect_plain_numbers<MonsterParams>("MonsterEditor", MonsterEditor::FIELDS, std::span{ MONSTEREDITOR_PLAIN_AT_HEAD });
}

// A field routed to a path its descriptor does not send it to stops the program and names it.
TEST(EditorFieldDeathTest, AMisroutedFieldAbortsNamingItsLabelAndThePath)
{
	EXPECT_DEATH(refuse_misrouted_field("Value", "ItemEditor::field_adjust"), "ItemEditor::field_adjust has no case for the field 'Value'");
}

// end of file: EditorFieldTest.cpp
