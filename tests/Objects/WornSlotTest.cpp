// file: WornSlotTest.cpp
//
// Where a worn item is worn, and that the data is the only thing that decides it.
//
// What it is for. Three behaviours granted ability bonuses - JewelryAmulet, Gauntlets
// and Girdle - and each equipped to a slot it named itself. There are more worn kinds
// than three, so "gauntlets" was doing duty as the generic worn thing with bonuses and
// every cloak and every pair of boots went onto the hands. The inventory screen had
// been patched to disagree, matching the cloak row by display name, so a cloak put on
// through the cloak row left that row empty, left the pack, and turned up on the
// gauntlets row with nothing saying so. EquipmentSlot::CLOAK and EquipmentSlot::BOOTS
// had never held an item.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=WornSlotTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/InventoryOperations.h"
#include "src/ItemCreator.h"
#include "src/ItemRegistry.h"
#include "src/Pickable.h"
#include "tests/mocks/MockGameContext.h"

class WornSlotTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		wearer.set_body_plan(mock.body_plans.get("humanoid"));
		wearer.set_strength(18);
		wearer.set_dexterity(10);
		// Putting a worn item on recomputes armour class, which reads this. A bare
		// Creature has none, so without it every case here dies in the pointer.
		wearer.armorClass = std::make_unique<ArmorClass>(10);
	}

	// Puts the named item in the pack and wears it the way the game does, through the
	// item's own behaviour, so the slot is chosen by the code under test.
	void wear_from_the_pack(const std::string& itemKey)
	{
		auto item = ItemCreator::create(itemKey, Vector2D{ 0, 0 }, ctx);
		ASSERT_NE(item, nullptr) << itemKey << " is not in the item registry";
		ASSERT_TRUE(item->behavior.has_value()) << itemKey << " has no behaviour to use";

		Item& placed = *item;
		const auto packed = InventoryOperations::add_item(wearer.inventoryData, std::move(item));
		ASSERT_TRUE(packed.has_value());

		EXPECT_TRUE(use_item(*placed.behavior, placed, wearer, ctx)) << "wearing " << itemKey << " was refused";
	}

	EquipmentSlot slot_in_the_data(const std::string& itemKey) const
	{
		return ctx.itemRegistry->get_params(itemKey).equipmentSlot;
	}

	MockGameContext mock{};
	GameContext ctx{};
	Creature wearer{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "wearer", ColorPairId::WHITE_BLACK } };
};

TEST_F(WornSlotTest, TheDataNamesASlotForEveryWornKind)
{
	EXPECT_EQ(slot_in_the_data("cloak_of_protection"), EquipmentSlot::CLOAK);
	EXPECT_EQ(slot_in_the_data("boots_of_speed"), EquipmentSlot::BOOTS);
	EXPECT_EQ(slot_in_the_data("gauntlets_of_ogre_power"), EquipmentSlot::GAUNTLETS);
	EXPECT_EQ(slot_in_the_data("girdle_of_hill_giant_strength"), EquipmentSlot::GIRDLE);
	EXPECT_EQ(slot_in_the_data("amulet_of_health"), EquipmentSlot::NECK);
	EXPECT_EQ(slot_in_the_data("helm_of_telepathy"), EquipmentSlot::HEAD);
}

TEST_F(WornSlotTest, AnItemNobodyWearsNamesNoSlot)
{
	// Without this the field could be filled in everywhere and mean nothing.
	EXPECT_EQ(slot_in_the_data("health_potion"), EquipmentSlot::NONE);
}

TEST_F(WornSlotTest, ACloakIsWornOnTheShoulders)
{
	wear_from_the_pack("cloak_of_protection");

	Item* onTheShoulders = wearer.get_equipped_item(EquipmentSlot::CLOAK);
	ASSERT_NE(onTheShoulders, nullptr) << "the cloak slot is still empty after putting a cloak on";
	EXPECT_EQ(onTheShoulders->actorData.name, "cloak of protection");
	EXPECT_EQ(wearer.get_equipped_item(EquipmentSlot::GAUNTLETS), nullptr) << "the cloak went onto the hands";
}

TEST_F(WornSlotTest, BootsAreWornOnTheFeet)
{
	wear_from_the_pack("boots_of_speed");

	Item* onTheFeet = wearer.get_equipped_item(EquipmentSlot::BOOTS);
	ASSERT_NE(onTheFeet, nullptr) << "the boots slot is still empty after putting boots on";
	EXPECT_EQ(wearer.get_equipped_item(EquipmentSlot::GAUNTLETS), nullptr) << "the boots went onto the hands";
}

TEST_F(WornSlotTest, GauntletsStillGoOnTheHands)
{
	wear_from_the_pack("gauntlets_of_ogre_power");

	Item* onTheHands = wearer.get_equipped_item(EquipmentSlot::GAUNTLETS);
	ASSERT_NE(onTheHands, nullptr);
	EXPECT_EQ(onTheHands->actorData.name, "gauntlets of ogre power");
}

TEST_F(WornSlotTest, ACloakAndGauntletsAreWornAtOnce)
{
	// The player-visible repair: both occupy their own slot, so neither displaces the
	// other and neither leaves the character's gear without saying so.
	wear_from_the_pack("cloak_of_protection");
	wear_from_the_pack("gauntlets_of_ogre_power");

	EXPECT_NE(wearer.get_equipped_item(EquipmentSlot::CLOAK), nullptr);
	EXPECT_NE(wearer.get_equipped_item(EquipmentSlot::GAUNTLETS), nullptr);
}

TEST_F(WornSlotTest, AWornItemIsNeverLostBetweenThePackAndTheSlot)
{
	// The symptom reported: the cloak left the pack and the slot it was put in stayed
	// empty, so it was gone from everything the player could see.
	wear_from_the_pack("cloak_of_protection");

	EXPECT_EQ(InventoryOperations::get_item_count(wearer.inventoryData), 0u) << "it never left the pack";
	EXPECT_NE(wearer.get_equipped_item(EquipmentSlot::CLOAK), nullptr) << "it left the pack and reached no slot";
}
