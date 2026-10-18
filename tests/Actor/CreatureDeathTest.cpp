// file: CreatureDeathTest.cpp
// What a monster leaves behind.
//
// Creature::die is the death every monster in the game actually goes through:
// combat reaches it through take_damage_and_check_death, and Player overrides it
// with the defeat screen. What it does, in order: says the creature is dead,
// hands its experience to the player, drops what it was carrying and what it wore
// onto the floor where it fell, and leaves a corpse in its place.
//
// It is reached here by calling die(). src/DeathHandler.cpp held a second copy of
// this body that nothing constructed, and testing that copy is what this file did
// until 2026-09-30 - it measured a function the game never called. Those files are
// gone now.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CreatureDeathTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Player.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int MONSTER_XP = 45;
constexpr Vector2D WHERE_IT_FELL{ 7, 9 };
constexpr Vector2D PICKED_UP_OVER_THERE{ 2, 2 };
} // namespace

class CreatureDeathTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		// A player always has a class, and its experience table is read when a kill pays it.
		player->set_creature_class(CreatureClass::FIGHTER);
		player->set_strength(14);

		monster = std::make_unique<Creature>(
			WHERE_IT_FELL, ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK });
		monster->experienceReward = std::make_unique<ExperienceReward>(MONSTER_XP);
		monster->healthPool = std::make_unique<HealthPool>(8);
		monster->armorClass = std::make_unique<ArmorClass>(6);
		monster->set_strength(14);
		monster->set_corpse("dead orc", 80, "It's tough and stringy.", CorpseEdibility::EDIBLE);
	}

	// Puts an item in the monster's pack, as loot it was carrying rather than
	// wearing. It is made somewhere else on purpose: a creature carries what it
	// picked up elsewhere, so the drop has to move it to where the body lands.
	Item& carried(std::string_view key)
	{
		auto item = ItemCreator::create(key, PICKED_UP_OVER_THERE, ctx);
		Item* held = item.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item(monster->inventoryData, std::move(item));
		return *held;
	}

	// The path combat takes: take_damage_and_check_death calls die() the moment
	// the pool empties, so this is the same entry point a killing blow uses.
	void kill_it()
	{
		monster->die(ctx);
	}

	bool floor_holds(const Item& item) const
	{
		auto is_the_item = [&item](const std::unique_ptr<Item>& dropped)
		{
			return dropped.get() == &item;
		};
		return std::ranges::any_of(ctx.floorInventory->items, is_the_item);
	}

	const Item* corpse_on_the_floor() const
	{
		auto is_a_corpse = [](const std::unique_ptr<Item>& dropped)
		{
			return dropped && dropped->actorData.name.starts_with("dead ");
		};
		const auto found = std::ranges::find_if(ctx.floorInventory->items, is_a_corpse);
		return found == ctx.floorInventory->items.end() ? nullptr : found->get();
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 1, 1 }) };
	std::unique_ptr<Creature> monster;
};

// The reward for the kill reaches the player.
TEST_F(CreatureDeathTest, TheKillerIsPaidTheExperience)
{
	const int before = player->get_xp();

	kill_it();

	EXPECT_EQ(player->get_xp(), before + MONSTER_XP) << "the kill paid other than the monster's worth";
}

// What it carried falls where it fell, so it can be picked up.
TEST_F(CreatureDeathTest, WhatItCarriedFallsWhereItDied)
{
	Item& loot = carried("health_potion");

	kill_it();

	EXPECT_TRUE(floor_holds(loot)) << "the pack was destroyed with the creature";
	EXPECT_EQ(loot.position, WHERE_IT_FELL) << "the loot did not land where the creature fell";
	EXPECT_TRUE(monster->inventoryData.items.empty()) << "the corpse still holds its pack";
}

// A body is left in its place, named for what it was.
TEST_F(CreatureDeathTest, ACorpseIsLeftBehindNamedForTheCreature)
{
	kill_it();

	const Item* corpse = corpse_on_the_floor();
	ASSERT_NE(corpse, nullptr) << "nothing was left where the creature stood";
	EXPECT_EQ(corpse->actorData.name, "dead orc");
	EXPECT_EQ(corpse->position, WHERE_IT_FELL);
}

// A corpse weighs what the creature's body weighs, which is what makes carrying
// one a decision rather than free food.
TEST_F(CreatureDeathTest, ACorpseWeighsWhatTheBodyWeighed)
{
	const int bodyWeight = monster->get_corpse_weight();
	ASSERT_GT(bodyWeight, 0) << "this creature's body weighs nothing, so the test below proves nothing";

	kill_it();

	const Item* corpse = corpse_on_the_floor();
	ASSERT_NE(corpse, nullptr);
	EXPECT_EQ(corpse->enhancement.weight, bodyWeight);
}

// Everything it carried goes, not merely the first thing.
TEST_F(CreatureDeathTest, EveryCarriedItemFalls)
{
	Item& first = carried("health_potion");
	Item& second = carried("bread");

	kill_it();

	EXPECT_TRUE(floor_holds(first));
	EXPECT_TRUE(floor_holds(second)) << "only part of the pack was dropped";
}

// The floor has no cap: a level already holding a thousand items still takes everything
// the dead carried, and the corpse beside it.
TEST_F(CreatureDeathTest, ACrowdedFloorStillTakesEverythingTheDeadCarried)
{
	constexpr int ITEMS_ALREADY_THERE = 1000;
	for (int already = 0; already < ITEMS_ALREADY_THERE; ++already)
	{
		ctx.floorInventory->items.push_back(std::make_unique<Item>(PICKED_UP_OVER_THERE, ActorData{ TileRef{}, "pebble", ColorPairId::WHITE_BLACK }));
	}
	Item& first = carried("health_potion");
	Item& second = carried("bread");

	kill_it();

	EXPECT_TRUE(floor_holds(first)) << "a crowded floor destroyed the pack";
	EXPECT_TRUE(floor_holds(second));
	EXPECT_NE(corpse_on_the_floor(), nullptr) << "a crowded floor destroyed the corpse";
}

// What it wore falls with it: the sword in its hand and the armour on its back land where
// it fell, no longer worn, and the body keeps nothing.
TEST_F(CreatureDeathTest, WhatItWoreFallsWhereItDied)
{
	monster->set_body_plan(mock.body_plans.get("humanoid"));
	auto sword = ItemCreator::create("long_sword", PICKED_UP_OVER_THERE, ctx);
	auto armour = ItemCreator::create("leather_armor", PICKED_UP_OVER_THERE, ctx);
	const Item& wielded = *sword;
	const Item& worn = *armour;
	monster->wear(std::move(sword), EquipmentSlot::RIGHT_HAND);
	monster->wear(std::move(armour), EquipmentSlot::BODY);

	kill_it();

	EXPECT_TRUE(floor_holds(wielded)) << "the sword it was wielding was destroyed with it";
	EXPECT_TRUE(floor_holds(worn)) << "the armour it was wearing was destroyed with it";
	EXPECT_EQ(wielded.position, WHERE_IT_FELL);
	EXPECT_EQ(worn.position, WHERE_IT_FELL);
	EXPECT_FALSE(wielded.has_state(ActorState::IS_EQUIPPED)) << "the dropped sword still counts as worn";
	EXPECT_TRUE(monster->equippedItems.empty()) << "the corpse still wears its gear";
}

// end of file: CreatureDeathTest.cpp
