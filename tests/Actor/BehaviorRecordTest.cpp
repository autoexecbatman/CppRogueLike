// file: BehaviorRecordTest.cpp
// An item behaviour saves every field it has and loads back exactly what it saved, and a
// record missing any field it should carry is refused rather than defaulted.
//
// What it is for. load_behavior read most fields behind a contains() guard, so a missing
// or misnamed key quietly became the field's default. That hid three key drifts that
// shipped - a weapon's hand requirement, a scroll's animation, a potion's amount - and
// would hide the next. The owner's rulings close it: CLAUDE.md's "use j.at() for required
// fields", and "no legacy save fallbacks should exist" (2026-09-21), so a loader has one
// shape to read.
//
// Both cases below are driven by what save_behavior actually writes, not by a list kept
// here, so a field added to a saver is checked without anyone remembering to add it.
// Every behaviour is built with values away from its defaults, since a default that
// survives a round trip proves nothing about whether the field was read.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=BehaviorRecordTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "src/BuffType.h"
#include "src/MagicalItemEffects.h"
#include "src/Pickable.h"
#include "src/TargetMode.h"
#include "src/Weapons.h"

using json = nlohmann::json;

namespace
{

struct NamedBehavior
{
	std::string name{};
	ItemBehavior behavior{};
};

// A stat-boost item whose every field differs from its default.
template <typename StatBoost>
StatBoost unusual_stat_boost()
{
	StatBoost statBoost{};
	statBoost.strBonus = 2;
	statBoost.dexBonus = 1;
	statBoost.conBonus = 3;
	statBoost.intBonus = 1;
	statBoost.wisBonus = 2;
	statBoost.chaBonus = 1;
	statBoost.isSetMode = true;
	statBoost.exceptionalStrength = 50;
	return statBoost;
}

// One of every behaviour that saves fields, each away from its defaults.
std::vector<NamedBehavior> every_behavior_with_fields()
{
	return {
		{ "consumable", Consumable{ ConsumableEffect::ADD_BUFF, 7, 3, BuffType::SANCTUARY, true } },
		{ "weapon", Weapon{ true, HandRequirement::TWO_HANDED, WeaponSize::LARGE, 18 } },
		{ "targeted scroll", TargetedScroll{ TargetMode::PICK_TILE_AOE, ScrollAnimation::LIGHTNING, 5, 9, 2, BuffType::SANCTUARY, 4 } },
		{ "gold", Gold{ 12 } },
		{ "food", Food{ 30 } },
		{ "corpse", CorpseFood{ 40 } },
		{ "armor", Armor{ 4 } },
		{ "magical helm", MagicalHelm{ MagicalEffect::BRILLIANCE, 2 } },
		{ "magical ring", MagicalRing{ MagicalEffect::PROTECTION, 1 } },
		{ "gauntlets", unusual_stat_boost<Gauntlets>() },
		{ "girdle", unusual_stat_boost<Girdle>() },
		{ "jewelry amulet", unusual_stat_boost<JewelryAmulet>() },
	};
}

// The fields of a behaviour record that carry an enum.
constexpr std::array<std::string_view, 7> ENUM_FIELDS = { "type", "effect", "buffType", "handRequirement", "weaponSize", "targetMode", "scrollAnimation" };

// Whether a record's key is one of those. The array is a file-scope constant and this
// takes no capture: MSVC 14.51 gives a lambda that captures a function-local constexpr
// array its own copy, so ranges::find returns an iterator into one object while end()
// names another, and the debug iterator check aborts the process on the mismatch.
bool is_enum_field(std::string_view key)
{
	return std::ranges::find(ENUM_FIELDS, key) != ENUM_FIELDS.end();
}

} // namespace

// The record names what it holds, and every enum in it, so what a record means does not
// depend on the order of an enum nobody consults when editing it.
TEST(BehaviorRecordTest, EveryEnumInTheRecordIsANameNotANumber)
{
	for (const NamedBehavior& named : every_behavior_with_fields())
	{
		json saved;
		save_behavior(named.behavior, saved);

		for (const auto& [key, value] : saved.items())
		{
			if (is_enum_field(key))
			{
				EXPECT_TRUE(value.is_string()) << "a " << named.name << " saved \"" << key << "\" as a number";
			}
		}
	}
}

// A behaviour this build cannot place is refused, rather than becoming whichever one
// the number happens to land on.
TEST(BehaviorRecordTest, AnUnknownBehaviourNameIsRefused)
{
	EXPECT_THROW((void)load_behavior(json{ { "type", "wand_of_wonder" } }), std::runtime_error);
}

// And one with no name at all, by the field read itself.
TEST(BehaviorRecordTest, ARecordWithNoBehaviourNameIsRefused)
{
	EXPECT_ANY_THROW((void)load_behavior(json::object()));
}

TEST(BehaviorRecordTest, EveryFieldASaveWritesIsRequiredToLoad)
{
	for (const NamedBehavior& named : every_behavior_with_fields())
	{
		json saved;
		save_behavior(named.behavior, saved);

		for (const auto& [key, value] : saved.items())
		{
			// The type is what picks the loader; the loader's own check refuses its absence.
			if (key == "type")
			{
				continue;
			}
			json missingOne = saved;
			missingOne.erase(key);
			EXPECT_ANY_THROW(load_behavior(missingOne))
				<< "a " << named.name << " record without \"" << key << "\" loaded quietly";
		}
	}
}

// A save, a load and a save again compares the saver against itself: one that writes the
// wrong name writes it both times and the comparison passes. These two ask what the
// record says and what came back, which is what catches a saver.
TEST(BehaviorRecordTest, EveryBehaviourComesBackAsItsOwnKind)
{
	for (const NamedBehavior& named : every_behavior_with_fields())
	{
		json saved;
		save_behavior(named.behavior, saved);

		const ItemBehavior loaded = load_behavior(saved);

		EXPECT_EQ(loaded.index(), named.behavior.index())
			<< "a " << named.name << " came back as a different behaviour";
	}
}

TEST(BehaviorRecordTest, APotionsRecordNamesItsEffectAndItsBuff)
{
	json saved;
	save_behavior(Consumable{ ConsumableEffect::ADD_BUFF, 7, 3, BuffType::SANCTUARY, true }, saved);

	EXPECT_EQ(saved.at("type"), "consumable");
	EXPECT_EQ(saved.at("effect"), "add_buff");
	EXPECT_EQ(saved.at("buffType"), "sanctuary");
}

TEST(BehaviorRecordTest, EveryFieldSurvivesARoundTrip)
{
	for (const NamedBehavior& named : every_behavior_with_fields())
	{
		json saved;
		save_behavior(named.behavior, saved);

		json savedAgain;
		save_behavior(load_behavior(saved), savedAgain);

		EXPECT_EQ(savedAgain, saved)
			<< "a " << named.name << " came back different: a field was read under another name, or not at all";
	}
}
