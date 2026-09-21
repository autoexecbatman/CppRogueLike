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

#include <string>
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

} // namespace

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
