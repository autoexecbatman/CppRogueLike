#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "src/Actor/Creature.h"
#include "src/Actor/EquipmentSlot.h"
#include "src/Actor/Item.h"
#include "src/Colors/Colors.h"

// Equipment lives on Creature, so any creature with a body plan can wear
// something. These tests cover get_equipped_item, which had no caller in the
// suite before the storage moved off Player.
class EquipmentStorageTest : public ::testing::Test
{
protected:
	std::unique_ptr<Creature> creature;

	void SetUp() override
	{
		ActorData data{ TileRef{}, "orc", WHITE_BLACK_PAIR };
		creature = std::make_unique<Creature>(Vector2D{ 0, 0 }, data);
		creature->set_body_plan({ EquipmentSlot::RIGHT_HAND, EquipmentSlot::BODY });
	}

	void wear(std::string_view name, EquipmentSlot slot)
	{
		auto item = std::make_unique<Item>(
			Vector2D{}, ActorData{ TileRef{}, std::string(name), WHITE_BLACK_PAIR });
		creature->equippedItems.push_back(EquippedItem(std::move(item), slot));
	}
};

// An empty slot answers with nothing rather than with whatever is nearby.
TEST_F(EquipmentStorageTest, EmptySlotHoldsNothing)
{
	EXPECT_EQ(creature->get_equipped_item(EquipmentSlot::RIGHT_HAND), nullptr);
	EXPECT_EQ(creature->get_equipped_item(EquipmentSlot::BODY), nullptr);
}

// The item that was put in a slot is the item that comes back out of it.
TEST_F(EquipmentStorageTest, WornItemIsFoundInItsSlot)
{
	wear("long sword", EquipmentSlot::RIGHT_HAND);

	Item* found = creature->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	ASSERT_NE(found, nullptr);
	EXPECT_EQ(found->actorData.name, "long sword");
}

// Slots are distinct. A sword in hand is not armor on the body, and asking
// for one must not return the other.
TEST_F(EquipmentStorageTest, SlotsDoNotBleedIntoEachOther)
{
	wear("long sword", EquipmentSlot::RIGHT_HAND);
	wear("chain mail", EquipmentSlot::BODY);

	Item* hand = creature->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	Item* body = creature->get_equipped_item(EquipmentSlot::BODY);

	ASSERT_NE(hand, nullptr);
	ASSERT_NE(body, nullptr);
	EXPECT_EQ(hand->actorData.name, "long sword");
	EXPECT_EQ(body->actorData.name, "chain mail");
	EXPECT_NE(hand, body);
}

// A slot nothing was put in stays empty even when other slots are filled.
TEST_F(EquipmentStorageTest, UnfilledSlotStaysEmpty)
{
	wear("long sword", EquipmentSlot::RIGHT_HAND);

	EXPECT_EQ(creature->get_equipped_item(EquipmentSlot::BODY), nullptr);
	EXPECT_EQ(creature->get_equipped_item(EquipmentSlot::HEAD), nullptr);
}

// What a creature strikes with has one answer, and it comes from the slot when
// something is in it and from the body when nothing is. Before this, a monster
// carried a display string that nothing could resolve to a real item.
TEST_F(EquipmentStorageTest, BodyProvidesTheAttackWhenNoWeaponIsHeld)
{
	creature->set_natural_attack("Claws");

	EXPECT_EQ(creature->get_attack_name(), "Claws");
}

TEST_F(EquipmentStorageTest, HeldWeaponNamesTheAttack)
{
	creature->set_natural_attack("Claws");
	wear("long sword", EquipmentSlot::RIGHT_HAND);

	EXPECT_EQ(creature->get_attack_name(), "long sword");
}

// A creature with hands, nothing in them and no natural attack is unarmed
// rather than nameless.
TEST_F(EquipmentStorageTest, EmptyHandedAndBodilessIsUnarmed)
{
	EXPECT_EQ(creature->get_attack_name(), "unarmed");
}

// Armor is not what you hit with. Only the main hand names the attack.
TEST_F(EquipmentStorageTest, WornArmorDoesNotNameTheAttack)
{
	creature->set_natural_attack("Claws");
	wear("chain mail", EquipmentSlot::BODY);

	EXPECT_EQ(creature->get_attack_name(), "Claws");
}
