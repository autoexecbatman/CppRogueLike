// file: ThiefSkillCharacterTest.cpp
// A character's thief skills: the race it was born with, the armour on its body
// and the points it spent, read off Player rather than off the tables.
//
// The tables themselves are ThiefSkillsTest's. What is checked here is that the
// character feeds them the right three things - Table 27's column for its race,
// Table 28's row for the Dexterity it is carrying now, Table 29's column for what
// is in its body slot - and that the points survive a save.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ThiefSkillCharacterTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>

#include <nlohmann/json.hpp>

#include "src/ArmorClass.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/ThiefSkills.h"

class ThiefSkillCharacterTest : public ::testing::Test
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
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_strength(12);
		player->set_dexterity(17);
		player->set_constitution(10);
		player->set_intelligence(10);
		player->set_wisdom(10);
		player->set_charisma(10);

		player->playerClassState = Player::PlayerClassState::ROGUE;
		player->set_creature_class(CreatureClass::ROGUE);
		player->playerClass = "Rogue";
		player->playerRaceState = Player::PlayerRaceState::DWARF;
		player->playerRace = "Dwarf";

		ctx = game.context();
		ctx.playerOwner = &player;
	}

	// Puts a suit of armour on the way the inventory screen does.
	void wear(std::string_view key)
	{
		auto item = ItemCreator::create(key, player->position, ctx);
		Item* carried = item.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item_to_inventory(
			player->inventoryData, std::move(item), *player, *ctx.dataManager);
		ASSERT_TRUE(added.has_value()) << key << " did not fit in the pack";
		use_item(*carried->behavior, *carried, *player, ctx);
		ASSERT_NE(player->get_equipped_item(EquipmentSlot::BODY), nullptr) << key << " was not put on";
	}

	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 0, 0 }) };
};

// Base 10, dwarf +10, Dexterity 17 +10, leather 0, nothing spent.
TEST_F(ThiefSkillCharacterTest, ADwarfRogueInLeatherReadsTheBookNumber)
{
	wear("leather_armor");

	EXPECT_EQ(player->thief_armor(), ThiefArmor::LEATHER);
	EXPECT_EQ(player->thief_skill(ThiefSkill::OPEN_LOCKS), 30);

	// Table 27's other dwarf entries reach the same way: +15 on traps, -10 on walls.
	EXPECT_EQ(player->thief_skill(ThiefSkill::FIND_REMOVE_TRAPS), 5 + 15);
	EXPECT_EQ(player->thief_skill(ThiefSkill::CLIMB_WALLS), 60 - 10);
}

// Table 29's "No Armor" column is a bonus, so an empty body slot is not the same
// as leather.
TEST_F(ThiefSkillCharacterTest, AnEmptyBodySlotIsTheNoArmorColumn)
{
	EXPECT_EQ(player->thief_armor(), ThiefArmor::NONE);

	// Move Silently: base 10, dwarf 0, Dexterity 17 +5, no armour +10.
	EXPECT_EQ(player->thief_skill(ThiefSkill::MOVE_SILENTLY), 25);

	wear("leather_armor");
	EXPECT_EQ(player->thief_skill(ThiefSkill::MOVE_SILENTLY), 15);
}

// Chain mail is the last column Table 29 prints.
TEST_F(ThiefSkillCharacterTest, ChainMailIsTheLastColumnAndItCosts)
{
	wear("chain_mail");

	EXPECT_EQ(player->thief_armor(), ThiefArmor::CHAIN_OR_RING_MAIL);
	EXPECT_EQ(player->thief_skill(ThiefSkill::OPEN_LOCKS), 30 - 10);
	EXPECT_EQ(player->thief_skill(ThiefSkill::MOVE_SILENTLY), 15 - 15);
}

// Nothing heavier has a column, and the book does not let a thief wear it: the
// character has no skill rather than a guessed one.
TEST_F(ThiefSkillCharacterTest, ArmorOffTheTableLeavesNoSkillAtAll)
{
	wear("plate_mail");

	EXPECT_FALSE(player->thief_armor().has_value());
	for (const ThiefSkill skill : ALL_THIEF_SKILL)
	{
		EXPECT_FALSE(player->thief_skill(skill).has_value()) << thief_skill_name(skill);
	}
}

TEST_F(ThiefSkillCharacterTest, OnlyARogueHasThiefSkillsAtAll)
{
	player->set_creature_class(CreatureClass::FIGHTER);
	player->playerClassState = Player::PlayerClassState::FIGHTER;

	for (const ThiefSkill skill : ALL_THIEF_SKILL)
	{
		EXPECT_FALSE(player->thief_skill(skill).has_value()) << thief_skill_name(skill);
	}
}

// The rule that replaced "base 10, +5 per level": a level raises nothing by
// itself. What rises is the points the player is handed and chooses to spend.
TEST_F(ThiefSkillCharacterTest, ALevelOnItsOwnRaisesNoSkill)
{
	wear("leather_armor");
	const int atFirstLevel = player->thief_skill(ThiefSkill::OPEN_LOCKS).value();

	player->set_creature_level(5);

	EXPECT_EQ(player->thief_skill(ThiefSkill::OPEN_LOCKS), atFirstLevel);

	// Four levels of grants, all thirty of each on this one skill up to the cap,
	// is what actually moves it.
	player->thiefSkillPoints.at(thief_skill_index(ThiefSkill::OPEN_LOCKS)) = 30;
	EXPECT_EQ(player->thief_skill(ThiefSkill::OPEN_LOCKS), atFirstLevel + 30);
}

// Gauntlets of dexterity carry a thief past the table's last printed row.
TEST_F(ThiefSkillCharacterTest, ADexterityPastTheTableStillAnswers)
{
	wear("leather_armor");
	player->set_dexterity(25);

	EXPECT_EQ(player->thief_skill(ThiefSkill::OPEN_LOCKS), 10 + 10 + 20);
}

TEST_F(ThiefSkillCharacterTest, ThePointsThePlayerSpentSurviveASave)
{
	player->thiefSkillPoints.at(thief_skill_index(ThiefSkill::HIDE_IN_SHADOWS)) = 22;
	player->thiefSkillPoints.at(thief_skill_index(ThiefSkill::OPEN_LOCKS)) = 8;

	nlohmann::json saved;
	player->save(saved);

	auto reloaded = std::make_unique<Player>(Vector2D{ 0, 0 });
	reloaded->load(saved);

	EXPECT_EQ(reloaded->thiefSkillPoints.at(thief_skill_index(ThiefSkill::HIDE_IN_SHADOWS)), 22);
	EXPECT_EQ(reloaded->thiefSkillPoints.at(thief_skill_index(ThiefSkill::OPEN_LOCKS)), 8);
	EXPECT_EQ(reloaded->thief_skill(ThiefSkill::OPEN_LOCKS), 38);
}

// A save written before thief skills existed is refused rather than read as a
// character who spent nothing - the loaders throw on a missing field here.
TEST_F(ThiefSkillCharacterTest, ASaveWithoutThePointsIsRefused)
{
	nlohmann::json saved;
	player->save(saved);
	saved.erase("thiefSkillPoints");

	auto reloaded = std::make_unique<Player>(Vector2D{ 0, 0 });
	EXPECT_THROW(reloaded->load(saved), nlohmann::json::exception);
}

// end of file: ThiefSkillCharacterTest.cpp
