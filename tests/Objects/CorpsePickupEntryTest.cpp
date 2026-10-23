// file: CorpsePickupEntryTest.cpp
//
// A corpse picked up through each of the player's three ways of asking - the pick-up key,
// a left click on the player's own tile, and a click on the corpse that walks there and
// arrives in WALK_TO_PICKUP - gives one answer from one state (issues/0229).
//
// Each entry point gets a world of its own, built the same way: a character of the given
// class and Strength in its class's starting kit, and a monster killed on open floor. The
// click cases read the mouse where raylib leaves it with no window open, pixel 0,0, so the
// clicked tile is the map's corner and the world is laid out around it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=*CorpsePickupEntry*

#include <gtest/gtest.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InputHandler.h"
#include "src/InputSystem.h"
#include "src/InventoryOperations.h"
#include "src/Map.h"
#include "src/MonsterCreator.h"
#include "src/Pickup.h"
#include "src/Player.h"
#include "src/PlayerController.h"
#include "src/PlayerRaceState.h"
#include "src/Renderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// Where an unopened window leaves the mouse: pixel 0,0, world tile 0,0 with the camera at
// the origin.
const Vector2D UNDER_THE_MOUSE{ 0, 0 };
// Three tiles along the same row, so a click on the corpse is a walk of three steps.
const Vector2D WALK_START{ 3, 0 };
// Enough frames for the walk's three steps and its arrival.
constexpr int FRAMES_TO_ARRIVE = 6;

enum class Entry
{
	PICK_KEY,
	CLICK_OWN_TILE,
	WALK_TO_PICKUP
};

struct PickupOutcome
{
	bool isTaken{ false };
	int maxCarried{ 0 };
	int carriedBefore{ 0 };
	int corpseWeight{ 0 };
};

// One game state, built fresh for one entry point.
struct PickupWorld
{
	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
	Renderer renderer{};

	// One frame of the game loop's input step, as GameLoopCoordinator runs it.
	void read_one_frame_of_input()
	{
		ctx.inputHandler->reset_key();
		ctx.inputSystem->poll();
		ctx.inputHandler->key_store();
		ctx.inputHandler->key_listen(*ctx.inputSystem);
	}

	PickupOutcome pick_up_through(
		Entry entry,
		CreatureClass creatureClass,
		int strength,
		const std::string& monsterKey)
	{
		mock.map.init_tiles();
		for (int col = 0; col <= WALK_START.x; ++col)
		{
			mock.map.set_tile(Vector2D{ col, 0 }, TileType::FLOOR);
		}
		ctx.renderer = &renderer;
		// A click on the stairs takes them, so they stand off the row the test clicks on.
		ctx.stairs->position = Vector2D{ WALK_START.x, WALK_START.y + 2 };
		EXPECT_EQ(
			ctx.inputSystem->get_mouse_world_tile(
				renderer.get_camera_x(),
				renderer.get_camera_y(),
				renderer.get_tile_size()),
			UNDER_THE_MOUSE)
			<< "the mouse is not where this test lays the world out";

		mock.player = std::make_unique<Player>(
			entry == Entry::WALK_TO_PICKUP ? WALK_START : UNDER_THE_MOUSE);
		Player& player = ctx.player_concrete();
		player.set_creature_class(creatureClass);
		player.lifespan->race = PlayerRaceState::HUMAN;
		player.set_strength(strength);
		player.set_creature_level(1);
		player.experienceReward = std::make_unique<ExperienceReward>(0);
		player.healthPool = std::make_unique<HealthPool>(10);
		player.armorClass = std::make_unique<ArmorClass>(10);
		player.equip_class_starting_gear(ctx);

		// A monster killed where it stood leaves its own record's corpse.
		auto monster = MonsterCreator::create_from_params(
			UNDER_THE_MOUSE,
			ctx.monsterRegistry->get_params(monsterKey),
			ctx);
		monster->die(ctx);
		PickupOutcome outcome{
			.maxCarried =
				InventoryOperations::get_max_weight(player, *ctx.dataManager),
			.carriedBefore = InventoryOperations::get_total_weight(player),
		};
		// The corpse is the item that is food, whatever its record names it.
		const auto is_a_corpse = [](const Item* onFloor)
		{
			return onFloor->behavior.has_value() &&
				std::holds_alternative<CorpseFood>(*onFloor->behavior);
		};
		const std::vector<Item*> underfoot =
			InventoryOperations::items_at(*ctx.floorInventory, UNDER_THE_MOUSE);
		const auto found = std::ranges::find_if(underfoot, is_a_corpse);
		EXPECT_NE(found, underfoot.end()) << monsterKey << " left no corpse";
		Item* corpse = found == underfoot.end() ? nullptr : *found;
		outcome.corpseWeight =
			corpse == nullptr ? 0 : corpse->enhancement.weight;

		const bool isKey = entry == Entry::PICK_KEY;
		ctx.inputSystem->queue_key(
			isKey ? GameKey::PICK : GameKey::MOUSE_LEFT,
			isKey ? 'p' : 0);
		for (int frame = 0; frame < FRAMES_TO_ARRIVE; ++frame)
		{
			read_one_frame_of_input();
			player.controller->update(ctx);
		}

		// A heap asks which. Choosing the corpse's entry runs take_floor_item on it, which
		// is called here directly: ticking the menu would draw it, and nothing is loaded to draw.
		if (!ctx.menus->empty())
		{
			assert(corpse && "the heap holds no corpse to choose");
			Pickup::take_floor_item(player, *corpse, ctx);
		}
		outcome.isTaken = std::ranges::none_of(
			InventoryOperations::items_at(*ctx.floorInventory, UNDER_THE_MOUSE),
			is_a_corpse);
		return outcome;
	}
};
} // namespace

class CorpsePickupEntryTest :
	public ::testing::TestWithParam<std::tuple<CreatureClass, int>>
{
};

TEST_P(CorpsePickupEntryTest, EveryEntryPointGivesOneAnswer)
{
	const auto [creatureClass, strength] = GetParam();

	const PickupOutcome byKey =
		std::make_unique<PickupWorld>()->pick_up_through(
			Entry::PICK_KEY,
			creatureClass,
			strength,
			"goblin");
	const PickupOutcome byOwnTileClick =
		std::make_unique<PickupWorld>()->pick_up_through(
			Entry::CLICK_OWN_TILE,
			creatureClass,
			strength,
			"goblin");
	const PickupOutcome byWalk =
		std::make_unique<PickupWorld>()->pick_up_through(
			Entry::WALK_TO_PICKUP,
			creatureClass,
			strength,
			"goblin");

	EXPECT_EQ(byKey.isTaken, byOwnTileClick.isTaken)
		<< "the key and a click on the player's tile disagree";
	EXPECT_EQ(byKey.isTaken, byWalk.isTaken)
		<< "the key and a walk to the corpse disagree";
}

// The owner's report: a rogue at 21 of 110 pounds could not lift a dead rat, whose record
// weighed it at a placeholder 100. A tiny corpse fits: 21 + 5 is at most 110.
TEST(CorpsePickupRatTest, ARogueAtStrengthTenPicksUpADeadRat)
{
	const PickupOutcome outcome =
		std::make_unique<PickupWorld>()
			->pick_up_through(Entry::PICK_KEY, CreatureClass::ROGUE, 10, "rat");

	EXPECT_EQ(outcome.maxCarried, 110);
	EXPECT_EQ(outcome.carriedBefore, 21);
	EXPECT_TRUE(outcome.isTaken)
		<< "a dead rat weighing " << outcome.corpseWeight << " was refused";
}

INSTANTIATE_TEST_SUITE_P(
	RogueAndFighter,
	CorpsePickupEntryTest,
	::testing::Combine(
		::testing::Values(CreatureClass::ROGUE, CreatureClass::FIGHTER),
		::testing::Values(8, 10, 13, 16)));

// end of file: CorpsePickupEntryTest.cpp
