// file: CurseSystemTest.cpp
// Verifies that CurseSystem correctly applies per-turn penalties for equipped
// cursed items, and ignores blessed and uncursed items.

#include <gtest/gtest.h>

#include "../../src/Item.h"
#include "../../src/Player.h"
#include "../../src/ExperienceReward.h"
#include "../../src/Colors.h"
#include "../../src/GameContext.h"
#include "../../src/ItemClassification.h"
#include "../../src/ItemIdentification.h"
#include "../../src/CurseSystem.h"
#include "../../src/MessageSystem.h"
#include "../../src/Vector2D.h"

class CurseSystemTest : public ::testing::Test
{
protected:
	CurseSystemTest()
		: message_system()
		, curse_system()
	{
	}

	void SetUp() override
	{
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->set_dr(0);
		player->set_thaco(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(20);
		ctx.playerOwner = &player;
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
		auto item = std::make_unique<Item>(Vector2D{}, ActorData{ TileRef{}, std::string(name), ColorPairId::WHITE_BLACK });
		item->itemClass = itemClass;
		item->enhancement.blessing = blessing;
		player->equippedItems.push_back(EquippedItem(std::move(item), slot));
	}
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 10, 10 }) };
	MessageSystem message_system;
	CurseSystem curse_system;
	GameState game_state;
	GameContext ctx;
};

// Cursed weapon: message contains "weakens your aim"
TEST_F(CurseSystemTest, CursedWeaponAppliesPenalty)
{
	equip_cursed(ItemClass::SWORD, BlessingStatus::CURSED, "sword", EquipmentSlot::RIGHT_HAND);

	curse_system.apply_curses(*player, ctx);

	EXPECT_NE(message_system.get_current_message().find("weakens your aim"), std::string::npos);
}

// Cursed armor: message contains "deteriorates"
TEST_F(CurseSystemTest, CursedArmorAppliesPenalty)
{
	equip_cursed(ItemClass::ARMOR, BlessingStatus::CURSED, "plate armor", EquipmentSlot::BODY);

	curse_system.apply_curses(*player, ctx);

	EXPECT_NE(message_system.get_current_message().find("deteriorates"), std::string::npos);
}

// Cursed amulet: drains 1 HP per turn
TEST_F(CurseSystemTest, CursedAmuletDrainsHP)
{
	equip_cursed(ItemClass::AMULET, BlessingStatus::CURSED, "amulet of life drain", EquipmentSlot::NECK);

	const int hpBefore = player->get_hp();

	curse_system.apply_curses(*player, ctx);

	EXPECT_EQ(player->get_hp(), hpBefore - 1);
	EXPECT_NE(message_system.get_current_message().find("drains"), std::string::npos);
}

// Blessed item: must not trigger curse effects
TEST_F(CurseSystemTest, BlessedItemsIgnored)
{
	equip_cursed(ItemClass::SWORD, BlessingStatus::BLESSED, "blessed sword", EquipmentSlot::RIGHT_HAND);

	std::string msgBefore = message_system.get_current_message();
	curse_system.apply_curses(*player, ctx);

	EXPECT_EQ(message_system.get_current_message(), msgBefore);
}

// Uncursed item: must not trigger curse effects
TEST_F(CurseSystemTest, UncursedItemsIgnored)
{
	equip_cursed(ItemClass::SWORD, BlessingStatus::UNCURSED, "sword", EquipmentSlot::RIGHT_HAND);

	std::string msgBefore = message_system.get_current_message();
	curse_system.apply_curses(*player, ctx);

	EXPECT_EQ(message_system.get_current_message(), msgBefore);
}

// A drain that empties the pool: the player reads the drain, then the death, and
// the game is lost.
TEST_F(CurseSystemTest, LethalDrainReportsDrainThenDeathAndDefeats)
{
	// One hit point, so the amulet's one-point drain is lethal.
	player->healthPool = std::make_unique<HealthPool>(1);
	equip_cursed(ItemClass::AMULET, BlessingStatus::CURSED, "amulet of life drain", EquipmentSlot::NECK);

	curse_system.apply_curses(*player, ctx);

	ASSERT_GE(message_system.get_stored_message_count(), 2u);
	const size_t lastIndex = message_system.get_stored_message_count() - 1;
	EXPECT_EQ(message_system.get_attack_message_at(lastIndex - 1).front().text, "The curse drains 1 HP from you!");
	EXPECT_EQ(message_system.get_attack_message_at(lastIndex).front().text, "The curse has killed you!");
	EXPECT_EQ(game_state.get_game_status(), GameStatus::DEFEAT);
}

// A drain the player survives says nothing of death and leaves the game running.
TEST_F(CurseSystemTest, SurvivableDrainNeitherKillsNorDefeats)
{
	// Two hit points, so one drain leaves the player standing.
	player->healthPool = std::make_unique<HealthPool>(2);
	equip_cursed(ItemClass::AMULET, BlessingStatus::CURSED, "amulet of life drain", EquipmentSlot::NECK);

	curse_system.apply_curses(*player, ctx);

	EXPECT_EQ(player->get_hp(), 1);
	EXPECT_EQ(message_system.get_current_message(), "The curse drains 1 HP from you!");
	EXPECT_NE(game_state.get_game_status(), GameStatus::DEFEAT);
}
