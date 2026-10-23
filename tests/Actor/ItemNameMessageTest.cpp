// file: ItemNameMessageTest.cpp
// A message about an item names it as Item::get_name does: identified prefix by
// name, an identified curse appended. Each expected line is the site's own
// format filled with that name, so a site reading the bare base name fails.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ItemNameMessageTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/ItemEnhancements.h"
#include "src/Pickable.h"
#include "src/Pickup.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int ORC_MAX_HP = 20;
constexpr int STRENGTH_ENOUGH_TO_CARRY = 16;
} // namespace

class ItemNameMessageTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();

		orc = std::make_unique<Creature>(
			Vector2D{ 0, 0 },
			ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK });
		orc->experienceReward = std::make_unique<ExperienceReward>(0);
		orc->healthPool = std::make_unique<HealthPool>(ORC_MAX_HP);
		orc->armorClass = std::make_unique<ArmorClass>(10);
		orc->set_strength(STRENGTH_ENOUGH_TO_CARRY);
		orc->set_body_plan(
			{ EquipmentSlot::RIGHT_HAND, EquipmentSlot::RIGHT_RING });
	}

	// The item made from its record, carrying one prefix and the given blessing, all identified.
	std::unique_ptr<Item> enhanced(
		std::string_view key,
		PrefixType prefix,
		BlessingStatus blessing)
	{
		auto item = ItemCreator::create(key, orc->position, ctx);
		ItemEnhancement enhancement{};
		enhancement.prefix = prefix;
		enhancement.blessing = blessing;
		item->apply_enhancement(enhancement);
		item->identify_type();
		item->identify_enhancement();
		item->identify_buc();
		return item;
	}

	// The last finished message, its parts joined.
	std::string last_message() const
	{
		std::string text{};
		for (
			const LogMessage& part : mock.messages.get_attack_message_at(
				mock.messages.get_stored_message_count() - 1))
		{
			text += part.text;
		}
		return text;
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Creature> orc;
};

// Creature::unequip_item: "The {} is cursed and cannot be removed!"
TEST_F(ItemNameMessageTest, ACursedItemNamesItsWholeNameWhenItWillNotComeOff)
{
	ASSERT_TRUE(orc->equip_item(
		enhanced("long_sword", PrefixType::SHARP, BlessingStatus::CURSED),
		EquipmentSlot::RIGHT_HAND,
		ctx));

	EXPECT_FALSE(orc->unequip_item(EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_EQ(
		last_message(),
		"The Sharp long sword (cursed) is cursed and cannot be removed!");
}

// Pickup::take_floor_item: "You picked up the {}."
TEST_F(ItemNameMessageTest, PickingUpAnItemNamesItsWholeName)
{
	auto sword =
		enhanced("long_sword", PrefixType::SHARP, BlessingStatus::UNCURSED);
	Item& onTheFloor = *sword;
	InventoryOperations::add_item(*ctx.floorInventory, std::move(sword));

	Pickup::take_floor_item(*orc, onTheFloor, ctx);

	EXPECT_EQ(last_message(), "You picked up the Sharp long sword.");
}

// use_magical_equip, reached by a MagicalRing: "You put on the {}."
TEST_F(ItemNameMessageTest, PuttingOnARingNamesItsWholeName)
{
	auto ring = enhanced(
		"ring_of_fire_resistance",
		PrefixType::BLESSED,
		BlessingStatus::UNCURSED);
	Item& carried = *ring;
	[[maybe_unused]] const auto added =
		InventoryOperations::add_item_to_inventory(
			orc->inventoryData,
			std::move(ring),
			*orc,
			*ctx.dataManager);

	ASSERT_TRUE(use_item(*carried.behavior, carried, *orc, ctx));

	EXPECT_EQ(
		last_message(),
		"You put on the Blessed ring of fire resistance.");
}

// Creature::unequip_item, with no room in the pack: "You cannot carry the {} as well, and set it down."
TEST_F(ItemNameMessageTest, AnItemSetDownNamesItsWholeName)
{
	ASSERT_TRUE(orc->equip_item(
		enhanced("long_sword", PrefixType::SHARP, BlessingStatus::UNCURSED),
		EquipmentSlot::RIGHT_HAND,
		ctx));
	[[maybe_unused]] const auto added =
		InventoryOperations::add_item_to_inventory(
			orc->inventoryData,
			ItemCreator::create("plate_mail", orc->position, ctx),
			*orc,
			*ctx.dataManager);
	orc->set_strength(3);
	ASSERT_FALSE(
		InventoryOperations::is_within_weight_limit(
			*orc->get_equipped_item(EquipmentSlot::RIGHT_HAND),
			*orc,
			*ctx.dataManager))
		<< "the sword fits in the pack, so it is never set down";

	EXPECT_TRUE(orc->unequip_item(EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_EQ(
		last_message(),
		"You cannot carry the Sharp long sword as well, and set it down.");
}
