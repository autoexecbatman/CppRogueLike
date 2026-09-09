#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "src/Factories/MonsterCreator.h"
#include "src/Factories/ItemCreator.h"
#include "src/Actor/EquipmentSlot.h"

// A monster either wields an item or fights with its body. The two are
// separate fields because only one of them names something in items.json.
class MonsterEquipmentTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		MonsterCreator::load("data/content/monsters.json");
		ItemCreator::load("data/content/items.json");
	}

	static const MonsterParams::StartingItem* find_slot(const MonsterParams& params, EquipmentSlot slot)
	{
		auto match = std::ranges::find_if(params.equipment,
			[slot](const MonsterParams::StartingItem& entry) { return entry.slot == slot; });
		return match == params.equipment.end() ? nullptr : &*match;
	}
};

// The orc's Long Sword is a real item, so it names the items.json key rather
// than a display string.
TEST_F(MonsterEquipmentTest, WieldingMonsterCarriesAnItemKey)
{
	const MonsterParams& orc = MonsterCreator::get_params("orc");

	const MonsterParams::StartingItem* mainHand = find_slot(orc, EquipmentSlot::RIGHT_HAND);
	ASSERT_NE(mainHand, nullptr);
	EXPECT_EQ(mainHand->itemKey, "long_sword");
	EXPECT_TRUE(orc.naturalAttack.empty());
}

// A troll's claws are part of it. Nothing in items.json corresponds, so no
// slot is filled.
TEST_F(MonsterEquipmentTest, ClawedMonsterCarriesNothing)
{
	const MonsterParams& troll = MonsterCreator::get_params("troll");

	EXPECT_TRUE(troll.equipment.empty());
	EXPECT_EQ(troll.naturalAttack, "Claws");
}

// The archer's Longbow and the warden's Longsword were misspelled against
// items.json, which is why neither resolved.
TEST_F(MonsterEquipmentTest, MisspelledWeaponsResolveToRealKeys)
{
	const MonsterParams::StartingItem* bow =
		find_slot(MonsterCreator::get_params("archer"), EquipmentSlot::MISSILE_WEAPON);
	ASSERT_NE(bow, nullptr);
	EXPECT_EQ(bow->itemKey, "long_bow");

	const MonsterParams::StartingItem* blade =
		find_slot(MonsterCreator::get_params("dungeon_warden"), EquipmentSlot::RIGHT_HAND);
	ASSERT_NE(blade, nullptr);
	EXPECT_EQ(blade->itemKey, "long_sword");
}

// Every key a monster carries must exist in items.json. This is the guard the
// display-name field never had.
TEST_F(MonsterEquipmentTest, EveryStartingItemKeyIsKnown)
{
	const std::vector<std::string> allItemKeys = ItemCreator::get_all_keys();
	const std::set<std::string> knownItemKeys(allItemKeys.begin(), allItemKeys.end());

	for (const std::string& key : MonsterCreator::get_all_keys())
	{
		const MonsterParams& params = MonsterCreator::get_params(key);
		for (const MonsterParams::StartingItem& entry : params.equipment)
		{
			EXPECT_TRUE(knownItemKeys.contains(entry.itemKey))
				<< key << " carries unknown item key " << entry.itemKey;
			EXPECT_NE(entry.slot, EquipmentSlot::NONE)
				<< key << " carries " << entry.itemKey << " in no slot";
		}
	}
}
