// file: CreatureEquipmentUseTest.cpp
// An ordinary creature puts something on, takes it off, and drinks a potion.
//
// None of this would compile before 2026-09-28: equip_item, unequip_item and
// use_item all took a Player, so the verbs were the player's alone even though
// every piece of state they touch - equippedItems, inventoryData, the health
// pool - has lived on Creature since the equipment storage moved down. The
// Player in those signatures was left over from before that move.
//
// This is what the move is for, so this is what it has to show: a monster can
// hold, swap and use what it carries. Nothing here mentions Player.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CreatureEquipmentUseTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <variant>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Pickable.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int ORC_MAX_HP = 20;
constexpr int STRENGTH_ENOUGH_TO_CARRY = 16;
} // namespace

class CreatureEquipmentUseTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();

		orc = std::make_unique<Creature>(
			Vector2D{ 0, 0 }, ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK });
		orc->experienceReward = std::make_unique<ExperienceReward>(0);
		orc->healthPool = std::make_unique<HealthPool>(ORC_MAX_HP);
		orc->armorClass = std::make_unique<ArmorClass>(10);
		orc->set_strength(STRENGTH_ENOUGH_TO_CARRY);
		orc->set_body_plan({ EquipmentSlot::RIGHT_HAND, EquipmentSlot::BODY });
	}

	Item& carry(std::string_view key)
	{
		auto item = ItemCreator::create(key, orc->position, ctx);
		Item* carried = item.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item_to_inventory(
			orc->inventoryData, std::move(item), *orc, *ctx.dataManager);
		return *carried;
	}

	bool pack_holds(const Item& item) const
	{
		auto is_the_item = [&item](const std::unique_ptr<Item>& packed)
		{
			return packed.get() == &item;
		};
		return std::ranges::any_of(orc->inventoryData.items, is_the_item);
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Creature> orc;
};

TEST_F(CreatureEquipmentUseTest, ACreaturePutsAWeaponInItsHand)
{
	Item& sword = carry("long_sword");
	auto taken = InventoryOperations::remove_item(orc->inventoryData, sword);
	ASSERT_TRUE(taken.has_value());

	EXPECT_TRUE(orc->equip_item(std::move(*taken), EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_EQ(orc->get_equipped_item(EquipmentSlot::RIGHT_HAND), &sword);
	EXPECT_EQ(orc->get_attack_name(), "long sword") << "it is not striking with what it holds";
}

TEST_F(CreatureEquipmentUseTest, ACreatureTakesItBackOff)
{
	Item& sword = carry("long_sword");
	auto taken = InventoryOperations::remove_item(orc->inventoryData, sword);
	ASSERT_TRUE(taken.has_value());
	ASSERT_TRUE(orc->equip_item(std::move(*taken), EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_TRUE(orc->unequip_item(EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_EQ(orc->get_equipped_item(EquipmentSlot::RIGHT_HAND), nullptr);
	EXPECT_TRUE(pack_holds(sword)) << "it did not come back to the pack";
}

TEST_F(CreatureEquipmentUseTest, ACreatureDrinksAPotion)
{
	Item& potion = carry("health_potion");
	ASSERT_TRUE(potion.behavior.has_value()) << "the potion carries nothing to use";

	// Asked of the potion rather than written down, so a balance edit to the data
	// moves the expectation with it instead of failing.
	const int healsBy = std::get<Consumable>(*potion.behavior).amount;
	ASSERT_GT(healsBy, 0) << "this potion heals nothing, so the test below proves nothing";

	// Low enough that the whole draught fits: a wearer near full would clamp, and
	// then any amount at all would look correct.
	orc->set_hp(1);
	const int before = orc->get_hp();
	ASSERT_LE(before + healsBy, ORC_MAX_HP) << "the heal would clamp and hide its size";

	EXPECT_TRUE(use_item(*potion.behavior, potion, *orc, ctx));

	EXPECT_EQ(orc->get_hp(), before + healsBy) << "a monster was not healed by what the potion pours";
	EXPECT_FALSE(pack_holds(potion)) << "the potion was drunk and is still in the pack";
}

// A refused item goes back to the pack it came out of. Carrying it again adds no
// weight, so a carrier already past its limit - which taking off a Strength item
// can leave it - gets it back all the same instead of losing it.
TEST_F(CreatureEquipmentUseTest, AnOverloadedCreatureKeepsWhatItCannotPutOn)
{
	carry("plate_mail");
	Item& sword = carry("long_sword");
	auto taken = InventoryOperations::remove_item(orc->inventoryData, sword);
	ASSERT_TRUE(taken.has_value());
	orc->set_strength(3);
	ASSERT_TRUE(InventoryOperations::is_overloaded(*orc, *ctx.dataManager)) << "the carrier is within its limit, so the gate is never asked";
	ASSERT_FALSE(orc->can_equip(sword, EquipmentSlot::BODY)) << "the sword would go on, so the refusal is never reached";

	EXPECT_FALSE(orc->equip_item(std::move(*taken), EquipmentSlot::BODY, ctx));

	EXPECT_TRUE(pack_holds(sword)) << "the refused sword was destroyed rather than put back";
}

TEST_F(CreatureEquipmentUseTest, AnOverloadedCreatureKeepsTheBowItCannotDraw)
{
	orc->set_body_plan({ EquipmentSlot::RIGHT_HAND, EquipmentSlot::BODY, EquipmentSlot::MISSILE_WEAPON });
	carry("plate_mail");
	Item& bow = carry("composite_bow");
	auto taken = InventoryOperations::remove_item(orc->inventoryData, bow);
	ASSERT_TRUE(taken.has_value());
	orc->set_strength(3);
	ASSERT_TRUE(InventoryOperations::is_overloaded(*orc, *ctx.dataManager)) << "the carrier is within its limit, so the gate is never asked";
	ASSERT_TRUE(orc->can_equip(bow, EquipmentSlot::MISSILE_WEAPON)) << "the slot refuses the bow, so the draw check is never reached";
	ASSERT_FALSE(can_draw(*orc, bow)) << "a Strength 3 arm draws the bow, so nothing is refused";

	EXPECT_FALSE(orc->equip_item(std::move(*taken), EquipmentSlot::MISSILE_WEAPON, ctx));

	EXPECT_TRUE(pack_holds(bow)) << "the undrawable bow was destroyed rather than put back";
}

// A floor with no room refuses the drop before anything moves, so the pack keeps the
// item and holds no empty slot where it was.
TEST_F(CreatureEquipmentUseTest, DroppingOntoAFullFloorKeepsTheItem)
{
	Item& sword = carry("long_sword");
	InventoryOperations::set_inventory_capacity(*ctx.floorInventory, ctx.floorInventory->items.size());
	ASSERT_TRUE(InventoryOperations::is_inventory_full(*ctx.floorInventory));

	orc->drop(sword, ctx);

	auto is_empty_slot = [](const std::unique_ptr<Item>& packed)
	{
		return !packed;
	};
	EXPECT_TRUE(pack_holds(sword)) << "the sword left the pack for a floor with no room";
	EXPECT_TRUE(std::ranges::none_of(orc->inventoryData.items, is_empty_slot)) << "the pack holds an empty slot where the sword was";
}

// end of file: CreatureEquipmentUseTest.cpp
