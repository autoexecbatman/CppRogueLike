// file: ThiefSkillAllocationTest.cpp
// What the thief skill screen is handed, checked without opening one.
//
// thief_skill_allocation_for reads five things off the character as it stands at
// the moment of the call. The armour is the one that makes the call order matter:
// Player::on_new_game_start dresses the character before it asks, and a reader run
// the other way round would answer for a naked one. That ordering is stated in a
// comment and this is what checks it.
//
// The table numbers themselves are ThiefSkillsTest's and the character's own
// readers are ThiefSkillCharacterTest's. What is checked here is the handover.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ThiefSkillAllocationTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string_view>

#include "src/ArmorClass.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/MenuThiefSkills.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/ThiefSkills.h"

class ThiefSkillAllocationTest : public ::testing::Test
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

// The ordering Player::on_new_game_start states: the kit goes on, then the screen
// is asked. Asked first, it answers Table 29's "No Armor" column for a character
// that is about to be standing in leather.
TEST_F(ThiefSkillAllocationTest, TheAllocationReadsTheArmourOnTheBodyWhenItIsAsked)
{
	// Move Silently: base 10, dwarf 0, Dexterity 17 +5, no armour +10.
	const ThiefSkillAllocation beforeDressing = thief_skill_allocation_for(*player, 1);
	EXPECT_EQ(beforeDressing.score(ThiefSkill::MOVE_SILENTLY), 25);

	wear("leather_armor");

	// The same character, now in leather, which Table 29 gives no bonus at all.
	const ThiefSkillAllocation afterDressing = thief_skill_allocation_for(*player, 1);
	EXPECT_EQ(afterDressing.score(ThiefSkill::MOVE_SILENTLY), 15);
}

// Chain mail is the last column Table 29 prints, and it is the heaviest penalty a
// character can hand the screen.
TEST_F(ThiefSkillAllocationTest, TheArmourPenaltyReachesTheAllocationAtFullWeight)
{
	wear("chain_mail");

	const ThiefSkillAllocation allocation = thief_skill_allocation_for(*player, 1);

	// Open Locks: base 10, dwarf +10, Dexterity 17 +10, chain mail -10.
	EXPECT_EQ(allocation.score(ThiefSkill::OPEN_LOCKS), 20);
	// Move Silently: base 10, dwarf 0, Dexterity 17 +5, chain mail -15.
	EXPECT_EQ(allocation.score(ThiefSkill::MOVE_SILENTLY), 0);
}

// Armour heavier than Table 29 prints leaves the character with no skill at all,
// and the screen must not inherit that as a silent zero: the allocation is built
// unarmoured rather than stranding the points.
TEST_F(ThiefSkillAllocationTest, ArmourOffTheTableIsJudgedUnarmouredRatherThanStranded)
{
	wear("plate_mail");
	ASSERT_FALSE(player->thief_armor().has_value());

	const ThiefSkillAllocation allocation = thief_skill_allocation_for(*player, 1);

	// The No Armor column, the same answer an empty body slot gives.
	EXPECT_EQ(allocation.score(ThiefSkill::MOVE_SILENTLY), 25);
}

// What a level hands a thief: sixty points at first level, thirty at every one
// after it (Player's Handbook, PDF page 84).
TEST_F(ThiefSkillAllocationTest, TheGrantIsSixtyAtFirstLevelAndThirtyAfter)
{
	EXPECT_EQ(thief_skill_allocation_for(*player, 1).remaining_points(), 60);
	EXPECT_EQ(thief_skill_allocation_for(*player, 2).remaining_points(), 30);
	EXPECT_EQ(thief_skill_allocation_for(*player, 9).remaining_points(), 30);
}

// The reader answers for whoever is asked; deciding that only a rogue is asked at
// all belongs to the caller that opens the screen.
TEST_F(ThiefSkillAllocationTest, NoScreenOpensForACharacterWithNoThiefSkills)
{
	player->playerClassState = Player::PlayerClassState::FIGHTER;
	player->set_creature_class(CreatureClass::FIGHTER);

	const size_t before = ctx.menus->size();
	push_thief_skill_allocation(*player, 1, ctx);

	EXPECT_EQ(ctx.menus->size(), before) << "a fighter was offered thief skill points";
}
