// Checks that using a piece of equipment which cannot be put on or taken off does nothing
// and costs nothing.
//
// What it is for. Using a ring, helm, weapon or shield from the inventory toggles it on or
// off; the return value says whether a turn was spent. A cursed item "cannot be removed"
// (Player::unequip_item), so using a worn cursed item again must leave it worn, leave its
// equipped mark on it, and spend no turn - the player has done nothing.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=EquipmentUseTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>

#include "src/Actor.h"
#include <algorithm>

#include "src/ArmorClass.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/ItemEnhancements.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"

class EquipmentUseTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);
		game.itemRegistry.load(Paths::ITEMS);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->set_strength(16);
		player->set_dexterity(10);

		ctx = game.context();
		ctx.playerOwner = &player;
	}

	// Puts an item from the data on through the inventory screen's path, then curses it.
	Item& wear_cursed(std::string_view key, EquipmentSlot slot)
	{
		auto item = ItemCreator::create(key, player->position, ctx);
		Item* carried = item.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item_to_inventory(player->inventoryData, std::move(item), *player, *ctx.dataManager);
		use_item(*carried->behavior, *carried, *player, ctx);
		Item* worn = player->get_equipped_item(slot);
		EXPECT_EQ(worn, carried) << key << " was not put on";
		carried->enhancement.blessing = BlessingStatus::CURSED;
		return *carried;
	}

	// Puts an item from the data on through the inventory screen's path.
	Item& wear(std::string_view key, EquipmentSlot slot)
	{
		auto item = ItemCreator::create(key, player->position, ctx);
		Item* carried = item.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item_to_inventory(player->inventoryData, std::move(item), *player, *ctx.dataManager);
		use_item(*carried->behavior, *carried, *player, ctx);
		EXPECT_EQ(player->get_equipped_item(slot), carried) << key << " was not put on";
		return *carried;
	}

	// Whether the pack holds this item, and whether it still says it is worn.
	bool pack_holds(const Item& item) const
	{
		auto is_the_item = [&item](const std::unique_ptr<Item>& packed)
		{
			return packed.get() == &item;
		};
		return std::ranges::any_of(player->inventoryData.items, is_the_item);
	}

	// Uses a worn cursed item again and checks that nothing happened.
	void expect_nothing_happens(Item& item, EquipmentSlot slot)
	{
		EXPECT_FALSE(use_item(*item.behavior, item, *player, ctx)) << item.actorData.name << ": a failed removal spent a turn";
		EXPECT_EQ(player->get_equipped_item(slot), &item) << item.actorData.name << " came off";
		EXPECT_TRUE(item.has_state(ActorState::IS_EQUIPPED)) << item.actorData.name << " lost its equipped mark while worn";
	}

	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 0, 0 }) };
};

TEST_F(EquipmentUseTest, ACursedRingThatWillNotComeOffCostsNothing)
{
	Item& ring = wear_cursed("ring_of_protection_plus_1", EquipmentSlot::RIGHT_RING);

	expect_nothing_happens(ring, EquipmentSlot::RIGHT_RING);
}

TEST_F(EquipmentUseTest, ACursedHelmThatWillNotComeOffCostsNothing)
{
	Item& helm = wear_cursed("helm_of_telepathy", EquipmentSlot::HEAD);

	expect_nothing_happens(helm, EquipmentSlot::HEAD);
}

TEST_F(EquipmentUseTest, ACursedWeaponThatWillNotComeOffCostsNothing)
{
	Item& club = wear_cursed("club", EquipmentSlot::RIGHT_HAND);

	expect_nothing_happens(club, EquipmentSlot::RIGHT_HAND);
}

TEST_F(EquipmentUseTest, ACursedShieldThatWillNotComeOffCostsNothing)
{
	Item& shield = wear_cursed("small_shield", EquipmentSlot::LEFT_HAND);

	expect_nothing_happens(shield, EquipmentSlot::LEFT_HAND);
}

// An item taken off goes back to the pack without its equipped mark. Creature::drop
// searches the pack alone and trusts that, so a mark left behind would make a
// dropped item claim to be worn.
TEST_F(EquipmentUseTest, AnItemTakenOffLeavesItsMarkBehind)
{
	Item& armor = wear("leather_armor", EquipmentSlot::BODY);

	ASSERT_TRUE(armor.has_state(ActorState::IS_EQUIPPED)) << "it was not worn to begin with";
	ASSERT_FALSE(pack_holds(armor)) << "a worn item is owned by its slot, not by the pack";

	ASSERT_TRUE(player->unequip_item(EquipmentSlot::BODY, ctx));

	EXPECT_TRUE(pack_holds(armor)) << "it did not come back to the pack";
	EXPECT_FALSE(armor.has_state(ActorState::IS_EQUIPPED)) << "a packed item still claims to be worn";
}
