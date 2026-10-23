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

#include <nlohmann/json.hpp>

#include "src/EquipmentSlot.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>

#include "src/Ai.h"
#include "src/Alignment.h"
#include "src/Attitude.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/ContentRegistry.h"
#include "src/ContentRegistryIO.h"
#include "src/CreatureClass.h"
#include "src/CreatureSize.h"
#include "src/DamageInfo.h"
#include "src/IntelligenceRating.h"
#include "src/ItemClassification.h"
#include "src/MagicalItemEffects.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Renderer.h"
#include "src/TargetMode.h"
#include "src/Weapons.h"

namespace
{
// Encodes to a string and parses it back; the value must survive the trip.
template <typename EnumType, typename Encode, typename Parse>
void expect_round_trip(
	EnumType value,
	Encode encode,
	Parse parse,
	const char* label)
{
	const std::string_view encoded = encode(value);

	EXPECT_NE(encoded, "") << label << ": encoded to an empty string";
	EXPECT_EQ(parse(encoded), value)
		<< label << ": '" << encoded << "' did not parse back";
}
} // namespace

TEST(CodecRoundTripTest, TargetModeSurvivesEncoding)
{
	constexpr auto& modes = ALL_TARGET_MODE;

	for (const auto mode : modes)
	{
		expect_round_trip(
			mode,
			encode_target_mode,
			parse_target_mode,
			"TargetMode");
	}
}

TEST(CodecRoundTripTest, ScrollAnimationSurvivesEncoding)
{
	constexpr auto& animations = ALL_SCROLL_ANIMATION;

	for (const auto animation : animations)
	{
		expect_round_trip(
			animation,
			encode_scroll_animation,
			parse_scroll_animation,
			"ScrollAnimation");
	}
}

TEST(CodecRoundTripTest, HandRequirementSurvivesEncoding)
{
	constexpr auto& requirements = ALL_HAND_REQUIREMENT;

	for (const auto requirement : requirements)
	{
		expect_round_trip(
			requirement,
			encode_hand_requirement,
			parse_hand_requirement,
			"HandRequirement");
	}
}

TEST(CodecRoundTripTest, WeaponSizeSurvivesEncoding)
{
	constexpr auto& sizes = ALL_WEAPON_SIZE;

	for (const auto size : sizes)
	{
		expect_round_trip(
			size,
			encode_weapon_size,
			parse_weapon_size,
			"WeaponSize");
	}
}

TEST(CodecRoundTripTest, CreatureSizeSurvivesEncoding)
{
	for (const auto size : ALL_CREATURE_SIZE)
	{
		expect_round_trip(
			size,
			encode_creature_size,
			parse_creature_size,
			"CreatureSize");
	}
}

TEST(CodecRoundTripTest, IntelligenceRatingSurvivesEncoding)
{
	for (const auto rating : ALL_INTELLIGENCE_RATING)
	{
		expect_round_trip(
			rating,
			encode_intelligence_rating,
			parse_intelligence_rating,
			"IntelligenceRating");
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
		expect_round_trip(
			value,
			encode_item_class,
			parse_item_class,
			"ItemClass");
	}
}

TEST(CodecRoundTripTest, PickableTypeSurvivesEncoding)
{
	constexpr auto& values = ALL_PICKABLE_TYPE;

	for (const auto value : values)
	{
		expect_round_trip(
			value,
			encode_pickable_type,
			parse_pickable_type,
			"PickableType");
	}
}

TEST(CodecRoundTripTest, MagicalEffectSurvivesEncoding)
{
	constexpr auto& values = ALL_MAGICAL_EFFECT;

	for (const auto value : values)
	{
		expect_round_trip(
			value,
			encode_magical_effect,
			parse_magical_effect,
			"MagicalEffect");
	}
}

TEST(CodecRoundTripTest, HelmCommandSurvivesEncoding)
{
	constexpr auto& values = ALL_HELM_COMMAND;

	for (const auto value : values)
	{
		expect_round_trip(
			value,
			encode_helm_command,
			parse_helm_command,
			"HelmCommand");
	}
}

// A string no encoder produces must be refused rather than silently defaulted.
TEST(CodecRoundTripTest, ConsumableEffectSurvivesEncoding)
{
	constexpr auto& effects = ALL_CONSUMABLE_EFFECT;

	for (const auto effect : effects)
	{
		expect_round_trip(
			effect,
			encode_consumable_effect,
			parse_consumable_effect,
			"ConsumableEffect");
	}
}

// Which Ai a saved creature carries: a name in the save, so what it means does not
// depend on the order of an enum nobody reads when editing it.
TEST(CodecRoundTripTest, AiTypeSurvivesEncoding)
{
	constexpr std::array kinds = {
		AiType::MONSTER,
		AiType::RANGED_MONSTER,
		AiType::CONFUSED_MONSTER,
		AiType::SHOPKEEPER,
		AiType::MIMIC,
		AiType::SPIDER,
		AiType::WEB_SPINNER,
		AiType::GIANT_SPIDER,
	};

	for (const auto kind : kinds)
	{
		expect_round_trip(kind, encode_ai_type, parse_ai_type, "AiType");
	}
}

// How a monster's record says it feels about the player.
TEST(CodecRoundTripTest, AttitudeSurvivesEncoding)
{
	for (const auto attitude : ALL_ATTITUDE)
	{
		expect_round_trip(
			attitude,
			encode_attitude,
			parse_attitude,
			"Attitude");
	}
}

// A record names its Ai from these, and the one that wraps another is not among them.
TEST(CodecRoundTripTest, OnlyAConfusedAiIsNotAuthorable)
{
	EXPECT_FALSE(is_authorable_ai_type(AiType::CONFUSED_MONSTER));
	for (const auto kind : AUTHORABLE_AI_TYPES)
	{
		EXPECT_TRUE(is_authorable_ai_type(kind)) << encode_ai_type(kind);
		EXPECT_NE(kind, AiType::CONFUSED_MONSTER);
	}
	EXPECT_EQ(AUTHORABLE_AI_TYPES.size(), 7u)
		<< "every AiType but CONFUSED_MONSTER";
}

// Where a creature stands on the law/chaos axis, and on the good/evil one. Two axes
// rather than nine labels, so each one round-trips on its own.
TEST(CodecRoundTripTest, AlignmentSurvivesEncoding)
{
	constexpr std::array ethics = {
		Ethics::LAWFUL,
		Ethics::NEUTRAL,
		Ethics::CHAOTIC,
	};
	for (const auto axis : ethics)
	{
		expect_round_trip(axis, encode_ethics, parse_ethics, "Ethics");
	}

	constexpr std::array moralities = {
		Morality::GOOD,
		Morality::NEUTRAL,
		Morality::EVIL,
	};
	for (const auto axis : moralities)
	{
		expect_round_trip(axis, encode_morality, parse_morality, "Morality");
	}
}

// Which class a saved creature belongs to, MONSTER included - it is what everything that
// is not a player character carries, so a save that loses it loses the most common value.
TEST(CodecRoundTripTest, CreatureClassSurvivesEncoding)
{
	constexpr std::array classes = {
		CreatureClass::FIGHTER,
		CreatureClass::ROGUE,
		CreatureClass::CLERIC,
		CreatureClass::WIZARD,
		CreatureClass::MONSTER,
	};

	for (const auto creatureClass : classes)
	{
		expect_round_trip(
			creatureClass,
			encode_creature_class,
			parse_creature_class,
			"CreatureClass");
	}
}

// What an attacker's damage is, in a save and on the screen alike: damage_type_name is
// the only table, so this round trip also pins the label the resolver's log prints.
TEST(CodecRoundTripTest, DamageTypeSurvivesEncoding)
{
	constexpr std::array types = {
		DamageType::PHYSICAL,
		DamageType::FIRE,
		DamageType::COLD,
		DamageType::LIGHTNING,
		DamageType::POISON,
		DamageType::ACID,
		DamageType::MAGIC,
	};

	for (const auto type : types)
	{
		expect_round_trip(
			type,
			damage_type_name,
			parse_damage_type,
			"DamageType");
	}
}

// Every colour pair a record can carry.
TEST(CodecRoundTripTest, ColorPairSurvivesEncoding)
{
	for (const ColorPairId pair : ALL_COLOR_PAIR)
	{
		expect_round_trip(
			pair,
			color_pair_name,
			parse_color_pair,
			"ColorPairId");
	}
}

// The parser is the only schema this data has.
TEST(CodecRoundTripTest, UnknownStringsThrow)
{
	EXPECT_THROW((void)parse_color_pair("puce"), std::runtime_error);
	EXPECT_THROW((void)parse_damage_type("sonic"), std::runtime_error);
	EXPECT_THROW((void)parse_ethics("scrupulous"), std::runtime_error);
	EXPECT_THROW((void)parse_morality("saintly"), std::runtime_error);
	EXPECT_THROW((void)parse_creature_class("bard"), std::runtime_error);
	EXPECT_THROW((void)parse_target_mode("not_a_mode"), std::runtime_error);
	EXPECT_THROW(
		(void)parse_scroll_animation("not_an_animation"),
		std::runtime_error);
	EXPECT_THROW(
		(void)parse_hand_requirement("not_a_requirement"),
		std::runtime_error);
	EXPECT_THROW((void)parse_weapon_size("not_a_size"), std::runtime_error);
	EXPECT_THROW((void)parse_creature_size("enormous"), std::runtime_error);
	EXPECT_THROW(
		(void)parse_intelligence_rating("brilliant"),
		std::runtime_error);
	EXPECT_THROW((void)parse_buff_type("not_a_buff"), std::runtime_error);
	EXPECT_THROW((void)parse_helm_command("awake"), std::runtime_error);
	EXPECT_THROW((void)parse_item_class("not_a_class"), std::runtime_error);
	EXPECT_THROW((void)parse_pickable_type("not_a_type"), std::runtime_error);
	EXPECT_THROW(
		(void)parse_magical_effect("not_an_effect"),
		std::runtime_error);
	EXPECT_THROW((void)parse_ai_type("not_an_ai"), std::runtime_error);
	EXPECT_THROW((void)parse_attitude("grumpy"), std::runtime_error);
	EXPECT_THROW(
		(void)parse_consumable_effect("not_an_effect"),
		std::runtime_error);
}

// Added 2026-10-01, after docs/pair_symmetry.py reported equipment_slot as the one
// encode/parse pair in src/ that no test reached. parse_equipment and encode_equipment
// cannot be reached: they sit in MonsterRegistry.cpp's anonymous namespace.
TEST(CodecRoundTripTest, EveryEquipmentSlotRoundTrips)
{
	for (int index = 0; index <= static_cast<int>(EquipmentSlot::NONE); ++index)
	{
		const EquipmentSlot slot = static_cast<EquipmentSlot>(index);

		EXPECT_EQ(parse_equipment_slot(encode_equipment_slot(slot)), slot)
			<< "slot " << index
			<< " did not survive a round trip through its JSON spelling";
	}
}

// The property the round trip above cannot see. parse_equipment_slot is implemented
// by scanning for the slot whose encoding matches, so two slots sharing a spelling
// would make the second parse back as the first - every creature wearing it would
// load with the item in the wrong place, and the round trip would still pass for the
// first of the two.
TEST(CodecRoundTripTest, NoTwoEquipmentSlotsShareASpelling)
{
	std::vector<std::string_view> spellings;
	for (int index = 0; index <= static_cast<int>(EquipmentSlot::NONE); ++index)
	{
		spellings.push_back(
			encode_equipment_slot(static_cast<EquipmentSlot>(index)));
	}
	const size_t written = spellings.size();

	std::ranges::sort(spellings);
	const auto duplicates = std::ranges::unique(spellings);
	spellings.erase(duplicates.begin(), duplicates.end());

	EXPECT_EQ(spellings.size(), written)
		<< "two equipment slots encode to the same string";
}

// Every item sprite in the shipped content file survives a save and a load through the
// tile codec: same keys, same sheet, column and row.
TEST(CodecRoundTripTest, EveryItemSpriteSurvivesASaveAndALoad)
{
	const std::filesystem::path scratch =
		std::filesystem::temp_directory_path() /
		"codec_round_trip_item_tiles.json";
	ContentRegistry shipped{};
	ContentRegistryIO::load(shipped, Paths::CONTENT_TILES);
	ASSERT_FALSE(shipped.all_tiles().empty());

	ContentRegistryIO::save(shipped, scratch.string());
	ContentRegistry reloaded{};
	ContentRegistryIO::load(reloaded, scratch.string());
	std::filesystem::remove(scratch);

	EXPECT_EQ(reloaded.all_tiles(), shipped.all_tiles());

	// Both sides went through one reader, so they are also checked against the file's own records.
	std::ifstream shippedFile(Paths::resolve(Paths::CONTENT_TILES));
	const nlohmann::json records =
		nlohmann::json::parse(shippedFile).at("items");
	ASSERT_EQ(records.size(), shipped.all_tiles().size());
	for (const auto& [key, record] : records.items())
	{
		const TileRef tile = shipped.get_tile(key);
		EXPECT_EQ(static_cast<int>(tile.sheet), record.at("sheet").get<int>())
			<< key;
		EXPECT_EQ(tile.col, record.at("col").get<int>()) << key;
		EXPECT_EQ(tile.row, record.at("row").get<int>()) << key;
	}
}

// A misspelled slot in a data file fails at load rather than going nowhere.
TEST(CodecRoundTripTest, AnUnknownEquipmentSlotThrows)
{
	EXPECT_THROW((void)parse_equipment_slot("third_hand"), std::runtime_error);
}
