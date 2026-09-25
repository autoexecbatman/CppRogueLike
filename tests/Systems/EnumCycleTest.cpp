// file: EnumCycleTest.cpp
// Every enum the item editor cycles through offers all of its values, and the cycle
// wraps back to where it started.
//
// What it is for. The editor cycled each field with a count written as
// `static_cast<int>(SomeEnum::LAST_ONE_AT_THE_TIME) + 1`. Every value added after that
// line was written became unreachable, silently: the editor is how content is made here,
// so an item type it cannot select is an item type nobody can author. Two of the nine
// counts had gone stale - PickableType lost IDENTIFY_SCROLL and DUNGEON_KEY, and BuffType
// lost SANCTUARY, PROTECTION_FROM_EVIL, SILENCE and WEBBED, which is every buff added
// since the editor was written.
//
// What this can and cannot catch. `ALL_X[i] == static_cast<X>(i)` fails if a value is
// inserted in the middle or the list is reordered, because every later entry shifts. A
// value appended at the end and left out of the list is still silent - the residual risk
// of having no reflection, the same one CodecRoundTripTest names. The headers used to
// claim -Wswitch made that impossible; no build here passes -Werror, so it warns at most.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=EnumCycleTest.*

#include <gtest/gtest.h>

#include <array>
#include <set>
#include <string_view>

#include "src/BuffType.h"
#include "src/ItemClassification.h"
#include "src/MagicalItemEffects.h"
#include "src/Pickable.h"
#include "src/TargetMode.h"
#include "src/Weapons.h"

namespace
{

// Walks the cycle from the first value and requires it to visit every value the list
// declares, in that order, and then wrap.
template <typename EnumType, std::size_t Count, typename Next>
void expect_cycle_covers(const std::array<EnumType, Count>& all, Next next, std::string_view label)
{
	ASSERT_GT(all.size(), 1u) << label << ": a list of one cannot show a cycle";

	EnumType value = all.front();
	for (std::size_t step = 1; step < all.size(); ++step)
	{
		value = next(value);
		EXPECT_EQ(value, all[step])
			<< label << ": step " << step << " left the declared order";
	}

	EXPECT_EQ(next(value), all.front()) << label << ": the cycle did not wrap";
}

// The list is the enum's own order, so its index is its value. An enumerator inserted
// anywhere but the end shifts every later entry and fails here.
template <typename EnumType, std::size_t Count>
void expect_list_matches_enum(const std::array<EnumType, Count>& all, std::string_view label)
{
	for (std::size_t index = 0; index < all.size(); ++index)
	{
		EXPECT_EQ(all[index], static_cast<EnumType>(index))
			<< label << ": entry " << index << " is not the value with that number";
	}
}

} // namespace

TEST(EnumCycleTest, EveryCycleCoversItsEnumAndWraps)
{
	expect_cycle_covers(ALL_ITEM_CLASS, next_item_class, "ItemClass");
	expect_cycle_covers(ALL_PICKABLE_TYPE, next_pickable_type, "PickableType");
	expect_cycle_covers(ALL_CONSUMABLE_EFFECT, next_consumable_effect, "ConsumableEffect");
	expect_cycle_covers(ALL_BUFF_TYPE, next_buff_type, "BuffType");
	expect_cycle_covers(ALL_TARGET_MODE, next_target_mode, "TargetMode");
	expect_cycle_covers(ALL_SCROLL_ANIMATION, next_scroll_animation, "ScrollAnimation");
	expect_cycle_covers(ALL_HAND_REQUIREMENT, next_hand_requirement, "HandRequirement");
	expect_cycle_covers(ALL_WEAPON_SIZE, next_weapon_size, "WeaponSize");
	expect_cycle_covers(ALL_MAGICAL_EFFECT, next_magical_effect, "MagicalEffect");
}

TEST(EnumCycleTest, EveryListIsTheEnumsOwnOrder)
{
	expect_list_matches_enum(ALL_ITEM_CLASS, "ItemClass");
	expect_list_matches_enum(ALL_PICKABLE_TYPE, "PickableType");
	expect_list_matches_enum(ALL_CONSUMABLE_EFFECT, "ConsumableEffect");
	expect_list_matches_enum(ALL_BUFF_TYPE, "BuffType");
	expect_list_matches_enum(ALL_TARGET_MODE, "TargetMode");
	expect_list_matches_enum(ALL_SCROLL_ANIMATION, "ScrollAnimation");
	expect_list_matches_enum(ALL_HAND_REQUIREMENT, "HandRequirement");
	expect_list_matches_enum(ALL_WEAPON_SIZE, "WeaponSize");
	expect_list_matches_enum(ALL_MAGICAL_EFFECT, "MagicalEffect");
}

// The six values the stale counts had cut off, named so this stays a check on the defect
// rather than only on the mechanism that replaced it.
TEST(EnumCycleTest, TheValuesTheStaleCountsCutOffAreReachable)
{
	const std::set<PickableType> pickables(ALL_PICKABLE_TYPE.begin(), ALL_PICKABLE_TYPE.end());
	EXPECT_TRUE(pickables.contains(PickableType::IDENTIFY_SCROLL));
	EXPECT_TRUE(pickables.contains(PickableType::DUNGEON_KEY));

	const std::set<BuffType> buffs(ALL_BUFF_TYPE.begin(), ALL_BUFF_TYPE.end());
	EXPECT_TRUE(buffs.contains(BuffType::SANCTUARY));
	EXPECT_TRUE(buffs.contains(BuffType::PROTECTION_FROM_EVIL));
	EXPECT_TRUE(buffs.contains(BuffType::SILENCE));
	EXPECT_TRUE(buffs.contains(BuffType::WEBBED));
}
