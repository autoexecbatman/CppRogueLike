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

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Pickable.h"
#include "src/RandomDice.h"
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

// toggle_weapon on an id the pack does not hold equips nothing and moves nothing.
TEST_F(CreatureEquipmentUseTest, TogglingAWeaponThePackDoesNotHoldChangesNothing)
{
	Item& sword = carry("long_sword");
	const size_t packed = orc->inventoryData.items.size();

	EXPECT_FALSE(orc->toggle_weapon(sword.uniqueId + 1, EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_TRUE(pack_holds(sword));
	EXPECT_EQ(orc->inventoryData.items.size(), packed);
	EXPECT_EQ(orc->get_equipped_item(EquipmentSlot::RIGHT_HAND), nullptr);
}

// toggle_weapon takes a carried sword out of the pack into the hand asked for.
TEST_F(CreatureEquipmentUseTest, TogglingASwordPutsItInTheHandAskedFor)
{
	Item& sword = carry("long_sword");

	EXPECT_TRUE(orc->toggle_weapon(sword.uniqueId, EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_EQ(orc->get_equipped_item(EquipmentSlot::RIGHT_HAND), &sword);
	EXPECT_FALSE(pack_holds(sword));
}

// A bow goes to the missile slot whichever hand was asked for.
TEST_F(CreatureEquipmentUseTest, TogglingABowPutsItInTheMissileSlot)
{
	orc->set_body_plan({
		EquipmentSlot::RIGHT_HAND,
		EquipmentSlot::BODY,
		EquipmentSlot::MISSILE_WEAPON,
	});
	// A short bow has no strength rating, so the orc's arm is not what this measures.
	Item& bow = carry("short_bow");
	ASSERT_TRUE(orc->can_equip(bow, EquipmentSlot::MISSILE_WEAPON)) << "the slot refuses the bow, so the toggle is never reached";
	ASSERT_TRUE(can_draw(*orc, bow)) << "the orc cannot draw the bow, so the toggle is refused for another reason";

	EXPECT_TRUE(orc->toggle_weapon(bow.uniqueId, EquipmentSlot::RIGHT_HAND, ctx));

	EXPECT_EQ(orc->get_equipped_item(EquipmentSlot::MISSILE_WEAPON), &bow);
	EXPECT_EQ(orc->get_equipped_item(EquipmentSlot::RIGHT_HAND), nullptr);
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

	// The book's 2d4 + 2 (DMG PDF page 890), with the dice scripted to 3 and 2.
	mock.dice.set_next_roll(3);
	mock.dice.set_next_roll(2);
	const int healsBy = 3 + 2 + 2;

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
	orc->set_body_plan({
		EquipmentSlot::RIGHT_HAND,
		EquipmentSlot::BODY,
		EquipmentSlot::MISSILE_WEAPON,
	});
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

// The floor has no limit: a drop onto one already holding a thousand items still lands,
// and the pack holds no empty slot where the item was.
TEST_F(CreatureEquipmentUseTest, DroppingOntoACrowdedFloorStillLands)
{
	Item& sword = carry("long_sword");
	constexpr int ITEMS_ALREADY_THERE = 1000;
	for (int already = 0; already < ITEMS_ALREADY_THERE; ++already)
	{
		ctx.floorInventory->items.push_back(std::make_unique<Item>(orc->position, ActorData{ TileRef{}, "pebble", ColorPairId::WHITE_BLACK }));
	}

	orc->drop(sword, ctx);

	auto is_the_sword = [&sword](const std::unique_ptr<Item>& dropped)
	{
		return dropped.get() == &sword;
	};
	auto is_empty_slot = [](const std::unique_ptr<Item>& packed)
	{
		return !packed;
	};
	EXPECT_TRUE(std::ranges::any_of(ctx.floorInventory->items, is_the_sword)) << "a crowded floor refused the sword";
	EXPECT_FALSE(pack_holds(sword));
	EXPECT_TRUE(std::ranges::none_of(orc->inventoryData.items, is_empty_slot)) << "the pack holds an empty slot where the sword was";
}

// end of file: CreatureEquipmentUseTest.cpp
