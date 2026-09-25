// file: CodecRoundTripTest.cpp
// Round-trips every enum that has a JSON string form: encode a value, parse the
// string back, and require the same value.
//
// These codecs previously lived in ItemCreator's anonymous namespace, where no
// test could reach them, and three BuffType values had been unreadable for an
// unknown period with nothing to notice. Each pair now sits beside its enum,
// which is what makes this file possible.
//
// The value lists live beside their enums as ALL_*, because the item editor cycles
// through the same lists: one place to add a value, read by the editor and by this.
// Adding an enumerator and not adding it to its list leaves a gap in both, which is
// the residual risk of refusing a COUNT sentinel. EnumCycleTest narrows it to a value
// appended at the end, since anything inserted earlier shifts the list out of step
// with the enum's own numbering.

#include <gtest/gtest.h>

#include <array>

#include "src/Pickable.h"
#include "src/ItemClassification.h"
#include "src/MagicalItemEffects.h"
#include "src/Weapons.h"
#include "src/Ai.h"
#include "src/Alignment.h"
#include "src/BuffType.h"
#include "src/CreatureClass.h"
#include "src/DamageInfo.h"
#include "src/TargetMode.h"

namespace
{
// Encodes to a string and parses it back; the value must survive the trip.
template <typename EnumType, typename Encode, typename Parse>
void expect_round_trip(EnumType value, Encode encode, Parse parse, const char* label)
{
	const std::string_view encoded = encode(value);

	EXPECT_NE(encoded, "") << label << ": encoded to an empty string";
	EXPECT_EQ(parse(encoded), value) << label << ": '" << encoded << "' did not parse back";
}
} // namespace

TEST(CodecRoundTripTest, TargetModeSurvivesEncoding)
{
	constexpr auto& modes = ALL_TARGET_MODE;

	for (const auto mode : modes)
	{
		expect_round_trip(mode, encode_target_mode, parse_target_mode, "TargetMode");
	}
}

TEST(CodecRoundTripTest, ScrollAnimationSurvivesEncoding)
{
	constexpr auto& animations = ALL_SCROLL_ANIMATION;

	for (const auto animation : animations)
	{
		expect_round_trip(animation, encode_scroll_animation, parse_scroll_animation, "ScrollAnimation");
	}
}

TEST(CodecRoundTripTest, HandRequirementSurvivesEncoding)
{
	constexpr auto& requirements = ALL_HAND_REQUIREMENT;

	for (const auto requirement : requirements)
	{
		expect_round_trip(requirement, encode_hand_requirement, parse_hand_requirement, "HandRequirement");
	}
}

TEST(CodecRoundTripTest, WeaponSizeSurvivesEncoding)
{
	constexpr auto& sizes = ALL_WEAPON_SIZE;

	for (const auto size : sizes)
	{
		expect_round_trip(size, encode_weapon_size, parse_weapon_size, "WeaponSize");
	}
}

TEST(CodecRoundTripTest, BuffTypeSurvivesEncoding)
{
	constexpr auto& buffs = ALL_BUFF_TYPE;

	for (const auto buff : buffs)
	{
		expect_round_trip(buff, encode_buff_type, parse_buff_type, "BuffType");
	}
}

TEST(CodecRoundTripTest, ItemClassSurvivesEncoding)
{
	constexpr auto& values = ALL_ITEM_CLASS;

	for (const auto value : values)
	{
		expect_round_trip(value, encode_item_class, parse_item_class, "ItemClass");
	}
}

TEST(CodecRoundTripTest, PickableTypeSurvivesEncoding)
{
	constexpr auto& values = ALL_PICKABLE_TYPE;

	for (const auto value : values)
	{
		expect_round_trip(value, encode_pickable_type, parse_pickable_type, "PickableType");
	}
}

TEST(CodecRoundTripTest, MagicalEffectSurvivesEncoding)
{
	constexpr auto& values = ALL_MAGICAL_EFFECT;

	for (const auto value : values)
	{
		expect_round_trip(value, encode_magical_effect, parse_magical_effect, "MagicalEffect");
	}
}

// A string no encoder produces must be refused rather than silently defaulted.
TEST(CodecRoundTripTest, ConsumableEffectSurvivesEncoding)
{
	constexpr auto& effects = ALL_CONSUMABLE_EFFECT;

	for (const auto effect : effects)
	{
		expect_round_trip(effect, encode_consumable_effect, parse_consumable_effect, "ConsumableEffect");
	}
}

// Which Ai a saved creature carries: a name in the save, so what it means does not
// depend on the order of an enum nobody reads when editing it.
TEST(CodecRoundTripTest, AiTypeSurvivesEncoding)
{
	constexpr std::array kinds = { AiType::MONSTER, AiType::CONFUSED_MONSTER, AiType::SHOPKEEPER,
		AiType::MIMIC, AiType::SPIDER, AiType::WEB_SPINNER, AiType::GIANT_SPIDER };

	for (const auto kind : kinds)
	{
		expect_round_trip(kind, encode_ai_type, parse_ai_type, "AiType");
	}
}

// Where a creature stands on the law/chaos axis, and on the good/evil one. Two axes
// rather than nine labels, so each one round-trips on its own.
TEST(CodecRoundTripTest, AlignmentSurvivesEncoding)
{
	constexpr std::array ethics = { Ethics::LAWFUL, Ethics::NEUTRAL, Ethics::CHAOTIC };
	for (const auto axis : ethics)
	{
		expect_round_trip(axis, encode_ethics, parse_ethics, "Ethics");
	}

	constexpr std::array moralities = { Morality::GOOD, Morality::NEUTRAL, Morality::EVIL };
	for (const auto axis : moralities)
	{
		expect_round_trip(axis, encode_morality, parse_morality, "Morality");
	}
}

// Which class a saved creature belongs to, MONSTER included - it is what everything that
// is not a player character carries, so a save that loses it loses the most common value.
TEST(CodecRoundTripTest, CreatureClassSurvivesEncoding)
{
	constexpr std::array classes = { CreatureClass::FIGHTER, CreatureClass::ROGUE,
		CreatureClass::CLERIC, CreatureClass::WIZARD, CreatureClass::MONSTER };

	for (const auto creatureClass : classes)
	{
		expect_round_trip(creatureClass, encode_creature_class, parse_creature_class, "CreatureClass");
	}
}

// What an attacker's damage is, in a save and on the screen alike: damage_type_name is
// the only table, so this round trip also pins the label the resolver's log prints.
TEST(CodecRoundTripTest, DamageTypeSurvivesEncoding)
{
	constexpr std::array types = { DamageType::PHYSICAL, DamageType::FIRE, DamageType::COLD,
		DamageType::LIGHTNING, DamageType::POISON, DamageType::ACID, DamageType::MAGIC };

	for (const auto type : types)
	{
		expect_round_trip(type, damage_type_name, parse_damage_type, "DamageType");
	}
}

// The parser is the only schema this data has.
TEST(CodecRoundTripTest, UnknownStringsThrow)
{
	EXPECT_THROW((void)parse_damage_type("sonic"), std::runtime_error);
	EXPECT_THROW((void)parse_ethics("scrupulous"), std::runtime_error);
	EXPECT_THROW((void)parse_morality("saintly"), std::runtime_error);
	EXPECT_THROW((void)parse_creature_class("bard"), std::runtime_error);
	EXPECT_THROW((void)parse_target_mode("not_a_mode"), std::runtime_error);
	EXPECT_THROW((void)parse_scroll_animation("not_an_animation"), std::runtime_error);
	EXPECT_THROW((void)parse_hand_requirement("not_a_requirement"), std::runtime_error);
	EXPECT_THROW((void)parse_weapon_size("not_a_size"), std::runtime_error);
	EXPECT_THROW((void)parse_buff_type("not_a_buff"), std::runtime_error);
	EXPECT_THROW((void)parse_item_class("not_a_class"), std::runtime_error);
	EXPECT_THROW((void)parse_pickable_type("not_a_type"), std::runtime_error);
	EXPECT_THROW((void)parse_magical_effect("not_an_effect"), std::runtime_error);
	EXPECT_THROW((void)parse_ai_type("not_an_ai"), std::runtime_error);
	EXPECT_THROW((void)parse_consumable_effect("not_an_effect"), std::runtime_error);
}
