// file: CurseSystemTest.cpp
// Verifies that CurseSystem correctly applies per-turn penalties for equipped
// cursed items, and ignores blessed and uncursed items.

#include <gtest/gtest.h>

#include "../../src/Actor/Destructible.h"
#include "../../src/Actor/Item.h"
#include "../../src/ActorTypes/Player.h"
#include "../../src/Colors/Colors.h"
#include "../../src/Core/GameContext.h"
#include "../../src/Items/ItemClassification.h"
#include "../../src/Items/ItemIdentification.h"
#include "../../src/Systems/CurseSystem.h"
#include "../../src/Systems/MessageSystem.h"
#include "../../src/Utils/Vector2D.h"

class CurseSystemTest : public ::testing::Test
{
protected:
	CurseSystemTest()
		: player(Vector2D{ 10, 10 })
		, message_system()
		, curse_system()
	{
	}

	void SetUp() override
	{
		player.destructible = std::make_unique<Destructible>(20, 0, "your corpse", 0, 0, 10, std::make_unique<PlayerDeathHandler>());
		ctx.player = &player;
		ctx.messageSystem = &message_system;
		ctx.curseSystem = &curse_system;
		ctx.gameState = &game_state;
	}

	// Helper: make a cursed item and equip it, transferring ownership to player.
	void equip_cursed(
		ItemClass itemClass,
		BlessingStatus blessing,
		std::string_view name,
		EquipmentSlot slot)
	{
		auto item = std::make_unique<Item>(Vector2D{}, ActorData{ TileRef{}, std::string(name), WHITE_BLACK_PAIR });
		item->itemClass = itemClass;
		item->enhancement.blessing = blessing;
		player.equippedItems.push_back(EquippedItem(std::move(item), slot));
	}

	Player player;
	MessageSystem message_system;
	CurseSystem curse_system;
	GameState game_state;
	GameContext ctx;
};

// Cursed weapon: message contains "weakens your aim"
TEST_F(CurseSystemTest, CursedWeaponAppliesPenalty)
{
	equip_cursed(ItemClass::SWORD, BlessingStatus::CURSED, "sword", EquipmentSlot::RIGHT_HAND);

	curse_system.apply_curses(player, ctx);

	EXPECT_NE(message_system.get_current_message().find("weakens your aim"), std::string::npos);
}

// Cursed armor: message contains "deteriorates"
TEST_F(CurseSystemTest, CursedArmorAppliesPenalty)
{
	equip_cursed(ItemClass::ARMOR, BlessingStatus::CURSED, "plate armor", EquipmentSlot::BODY);

	curse_system.apply_curses(player, ctx);

	EXPECT_NE(message_system.get_current_message().find("deteriorates"), std::string::npos);
}

// Cursed amulet: drains 1 HP per turn
TEST_F(CurseSystemTest, CursedAmuletDrainsHP)
{
	equip_cursed(ItemClass::AMULET, BlessingStatus::CURSED, "amulet of life drain", EquipmentSlot::NECK);

	const int hpBefore = player.destructible->get_hp();

	curse_system.apply_curses(player, ctx);

	EXPECT_EQ(player.destructible->get_hp(), hpBefore - 1);
	EXPECT_NE(message_system.get_current_message().find("drains"), std::string::npos);
}

// Blessed item: must not trigger curse effects
TEST_F(CurseSystemTest, BlessedItemsIgnored)
{
	equip_cursed(ItemClass::SWORD, BlessingStatus::BLESSED, "blessed sword", EquipmentSlot::RIGHT_HAND);

	std::string msgBefore = message_system.get_current_message();
	curse_system.apply_curses(player, ctx);

	EXPECT_EQ(message_system.get_current_message(), msgBefore);
}

// Uncursed item: must not trigger curse effects
TEST_F(CurseSystemTest, UncursedItemsIgnored)
{
	equip_cursed(ItemClass::SWORD, BlessingStatus::UNCURSED, "sword", EquipmentSlot::RIGHT_HAND);

	std::string msgBefore = message_system.get_current_message();
	curse_system.apply_curses(player, ctx);

	EXPECT_EQ(message_system.get_current_message(), msgBefore);
}
