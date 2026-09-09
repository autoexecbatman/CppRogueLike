// file: CodecRoundTripTest.cpp
// Round-trips every enum that has a JSON string form: encode a value, parse the
// string back, and require the same value.
//
// These codecs previously lived in ItemCreator's anonymous namespace, where no
// test could reach them, and three BuffType values had been unreadable for an
// unknown period with nothing to notice. Each pair now sits beside its enum,
// which is what makes this file possible.
//
// The value lists are written out rather than derived: a list read from the code
// under test cannot disagree with it. Adding an enumerator without adding it here
// leaves a gap, which is the residual risk of refusing a COUNT sentinel.

#include <gtest/gtest.h>

#include <array>

#include "src/Actor/Pickable.h"
#include "src/Items/ItemClassification.h"
#include "src/Items/MagicalItemEffects.h"
#include "src/Items/Weapons.h"
#include "src/Systems/BuffType.h"
#include "src/Systems/TargetMode.h"

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
	constexpr std::array modes = { TargetMode::AUTO_NEAREST, TargetMode::PICK_TILE_SINGLE,
		TargetMode::PICK_TILE_AOE, TargetMode::FOV_BUFF };

	for (const auto mode : modes)
	{
		expect_round_trip(mode, encode_target_mode, parse_target_mode, "TargetMode");
	}
}

TEST(CodecRoundTripTest, ScrollAnimationSurvivesEncoding)
{
	constexpr std::array animations = { ScrollAnimation::NONE, ScrollAnimation::LIGHTNING,
		ScrollAnimation::EXPLOSION };

	for (const auto animation : animations)
	{
		expect_round_trip(animation, encode_scroll_animation, parse_scroll_animation, "ScrollAnimation");
	}
}

TEST(CodecRoundTripTest, HandRequirementSurvivesEncoding)
{
	constexpr std::array requirements = { HandRequirement::ONE_HANDED, HandRequirement::TWO_HANDED,
		HandRequirement::OFF_HAND_ONLY };

	for (const auto requirement : requirements)
	{
		expect_round_trip(requirement, encode_hand_requirement, parse_hand_requirement, "HandRequirement");
	}
}

TEST(CodecRoundTripTest, WeaponSizeSurvivesEncoding)
{
	constexpr std::array sizes = { WeaponSize::TINY, WeaponSize::SMALL, WeaponSize::MEDIUM,
		WeaponSize::LARGE, WeaponSize::GIANT };

	for (const auto size : sizes)
	{
		expect_round_trip(size, encode_weapon_size, parse_weapon_size, "WeaponSize");
	}
}

TEST(CodecRoundTripTest, BuffTypeSurvivesEncoding)
{
	constexpr std::array buffs = { BuffType::NONE, BuffType::INVISIBILITY, BuffType::BLESS,
		BuffType::SHIELD, BuffType::STRENGTH, BuffType::DEXTERITY, BuffType::CONSTITUTION,
		BuffType::INTELLIGENCE, BuffType::WISDOM, BuffType::CHARISMA, BuffType::SPEED,
		BuffType::FIRE_RESISTANCE, BuffType::COLD_RESISTANCE, BuffType::LIGHTNING_RESISTANCE,
		BuffType::POISON_RESISTANCE, BuffType::SLEEP, BuffType::HOLD_PERSON, BuffType::SANCTUARY,
		BuffType::PROTECTION_FROM_EVIL, BuffType::SILENCE, BuffType::WEBBED };

	for (const auto buff : buffs)
	{
		expect_round_trip(buff, encode_buff_type, parse_buff_type, "BuffType");
	}
}

TEST(CodecRoundTripTest, ItemClassSurvivesEncoding)
{
	constexpr std::array values = { ItemClass::DAGGER, ItemClass::SWORD, ItemClass::GREAT_SWORD,
		ItemClass::AXE, ItemClass::HAMMER, ItemClass::MACE,
		ItemClass::STAFF, ItemClass::BOW, ItemClass::CROSSBOW,
		ItemClass::ARMOR, ItemClass::SHIELD, ItemClass::HELMET,
		ItemClass::RING, ItemClass::AMULET, ItemClass::GAUNTLETS,
		ItemClass::GIRDLE, ItemClass::POTION, ItemClass::SCROLL,
		ItemClass::FOOD };

	for (const auto value : values)
	{
		expect_round_trip(value, encode_item_class, parse_item_class, "ItemClass");
	}
}

TEST(CodecRoundTripTest, PickableTypeSurvivesEncoding)
{
	constexpr std::array values = { PickableType::TARGETED_SCROLL, PickableType::TELEPORTER, PickableType::WEAPON,
		PickableType::SHIELD, PickableType::CONSUMABLE, PickableType::GOLD_COIN,
		PickableType::FOOD, PickableType::CORPSE_FOOD, PickableType::ARMOR,
		PickableType::MAGICAL_HELM, PickableType::MAGICAL_RING, PickableType::JEWELRY_AMULET,
		PickableType::GAUNTLETS, PickableType::GIRDLE, PickableType::QUEST_ITEM,
		PickableType::IDENTIFY_SCROLL, PickableType::DUNGEON_KEY };

	for (const auto value : values)
	{
		expect_round_trip(value, encode_pickable_type, parse_pickable_type, "PickableType");
	}
}

TEST(CodecRoundTripTest, MagicalEffectSurvivesEncoding)
{
	constexpr std::array values = { MagicalEffect::NONE, MagicalEffect::BRILLIANCE, MagicalEffect::TELEPORTATION,
		MagicalEffect::TELEPATHY, MagicalEffect::UNDERWATER_ACTION, MagicalEffect::FREE_ACTION,
		MagicalEffect::REGENERATION, MagicalEffect::INVISIBILITY, MagicalEffect::FIRE_RESISTANCE,
		MagicalEffect::COLD_RESISTANCE, MagicalEffect::SPELL_STORING };

	for (const auto value : values)
	{
		expect_round_trip(value, encode_magical_effect, parse_magical_effect, "MagicalEffect");
	}
}

// A string no encoder produces must be refused rather than silently defaulted.
// The parser is the only schema this data has.
TEST(CodecRoundTripTest, UnknownStringsThrow)
{
	EXPECT_THROW((void)parse_target_mode("not_a_mode"), std::runtime_error);
	EXPECT_THROW((void)parse_scroll_animation("not_an_animation"), std::runtime_error);
	EXPECT_THROW((void)parse_hand_requirement("not_a_requirement"), std::runtime_error);
	EXPECT_THROW((void)parse_weapon_size("not_a_size"), std::runtime_error);
	EXPECT_THROW((void)parse_buff_type("not_a_buff"), std::runtime_error);
	EXPECT_THROW((void)parse_item_class("not_a_class"), std::runtime_error);
	EXPECT_THROW((void)parse_pickable_type("not_a_type"), std::runtime_error);
	EXPECT_THROW((void)parse_magical_effect("not_an_effect"), std::runtime_error);
}
