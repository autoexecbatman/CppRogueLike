// file: DoorForcingTest.cpp
// Forcing a locked door, by Table 1 (PHB Chapter 1, Strength, PDF pages 30-31): "Numbers
// in parentheses are the chances (on 1d20) to open a locked, barred, or magically held
// door, but only one attempt per door can ever be made. If it fails, no further attempts
// by that character can succeed." Only rows from 18/91-99 up print one, so the row decides
// who may try, whatever the class.
//
// Each test walks the player into a locked door with a key queued on InputSystem, down the
// path a pressed key takes in play, with the d20 scripted.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DoorForcingTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "src/CreatureClass.h"
#include "src/GameContext.h"
#include "src/InputHandler.h"
#include "src/InputSystem.h"
#include "src/InventoryOperations.h"
#include "src/ItemCreator.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/Player.h"
#include "src/PlayerController.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A row of floor with the player on it and a locked door one step east.
constexpr int CORRIDOR_ROW = 5;
const Vector2D START{ 4, CORRIDOR_ROW };
const Vector2D DOOR{ 5, CORRIDOR_ROW };

const std::string FORCED{ "You force the door open! The crash echoes down the corridor." };
const std::string HELD{ "You slam into the door but it holds." };
const std::string TOO_WEAK{ "The door is locked. You are not strong enough to force it." };
const std::string ALREADY_FAILED{ "You have already failed to force this door." };
// A queued value no attempt in these tests is meant to read, so reading it back proves
// nothing rolled.
constexpr int UNREAD_D20 = 13;
} // namespace

class DoorForcingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		mock.map.init_tiles();
		for (int col = 1; col < 10; ++col)
		{
			mock.map.set_tile(Vector2D{ col, CORRIDOR_ROW }, TileType::FLOOR);
		}
		mock.map.set_tile(DOOR, TileType::CLOSED_DOOR);
		ASSERT_TRUE(mock.map.lock_door(DOOR));
		mock.player = std::make_unique<Player>(START);
	}

	// A character of this class and Strength, with an empty pack.
	void become(CreatureClass creatureClass, int strength, int exceptional)
	{
		Player& player = ctx.player_concrete();
		player.set_creature_class(creatureClass);
		player.set_strength(strength);
		player.set_exceptional_strength(exceptional);
	}

	// One step east into the door, as a pressed key reaches the controller.
	void walk_into_the_door()
	{
		ctx.inputSystem->queue_key(GameKey::RIGHT, 0);
		ctx.inputHandler->reset_key();
		ctx.inputSystem->poll();
		ctx.inputHandler->key_store();
		ctx.inputHandler->key_listen(*ctx.inputSystem);
		ctx.player_concrete().controller->update(ctx);
	}

	// The refusal spends nothing: the clock stands, the door stays locked and untried, and
	// the queued d20 is still the next one.
	void expect_refused_without_a_roll(int timeBefore)
	{
		EXPECT_EQ(ctx.gameState->get_time(), timeBefore) << "a refused attempt spent time";
		EXPECT_TRUE(ctx.map->is_door_locked(DOOR));
		EXPECT_EQ(ctx.player()->position, START);
		EXPECT_EQ(mock.dice.d20(), UNREAD_D20) << "a refused attempt rolled the d20";
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
};

// 17 and 18/90 print no parenthesised figure, so neither may try. A plain 18 fighter is
// refused too, where the class alone used to be enough.
TEST_F(DoorForcingTest, ARowWithNoLockedDoorChanceIsRefusedWithoutARoll)
{
	struct Character
	{
		int strength{ 0 };
		int exceptional{ 0 };
	};

	for (const Character character : { Character{ 17, 0 }, Character{ 18, 90 }, Character{ 18, 0 } })
	{
		become(CreatureClass::FIGHTER, character.strength, character.exceptional);
		mock.dice.set_next_roll(UNREAD_D20);
		const int timeBefore = ctx.gameState->get_time();

		walk_into_the_door();

		SCOPED_TRACE(testing::Message() << "Strength " << character.strength << "/" << character.exceptional);
		EXPECT_EQ(ctx.messageSystem->get_current_message(), TOO_WEAK);
		EXPECT_FALSE(ctx.map->has_resisted_forcing(DOOR)) << "a refusal used up the attempt";
		expect_refused_without_a_roll(timeBefore);
	}
}

// 18/91-99 prints 15(3): a 3 opens the door, and the door stays a door.
TEST_F(DoorForcingTest, EighteenNinetyOneOpensOnAThree)
{
	become(CreatureClass::FIGHTER, 18, 91);
	mock.dice.set_next_roll(3);
	const int timeBefore = ctx.gameState->get_time();

	walk_into_the_door();

	EXPECT_EQ(ctx.map->get_tile_type(DOOR), TileType::OPEN_DOOR) << "a forced door is open, not torn out";
	EXPECT_FALSE(ctx.map->is_door_locked(DOOR));
	EXPECT_EQ(ctx.messageSystem->get_current_message(), FORCED);
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore + TIME_UNITS_PER_ROUND);
}

TEST_F(DoorForcingTest, EighteenNinetyOneFailsOnAFour)
{
	become(CreatureClass::FIGHTER, 18, 91);
	mock.dice.set_next_roll(4);
	const int timeBefore = ctx.gameState->get_time();

	walk_into_the_door();

	EXPECT_EQ(ctx.map->get_tile_type(DOOR), TileType::CLOSED_DOOR);
	EXPECT_TRUE(ctx.map->is_door_locked(DOOR));
	EXPECT_EQ(ctx.messageSystem->get_current_message(), HELD);
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore + TIME_UNITS_PER_ROUND);
}

// 18/00 prints 16(6): the issue's own pair, a 6 opens.
TEST_F(DoorForcingTest, EighteenHundredOpensOnASix)
{
	become(CreatureClass::FIGHTER, 18, 100);
	mock.dice.set_next_roll(6);

	walk_into_the_door();

	EXPECT_EQ(ctx.map->get_tile_type(DOOR), TileType::OPEN_DOOR);
	EXPECT_EQ(ctx.messageSystem->get_current_message(), FORCED);
}

// And a 7 fails, which uses up the one attempt: the next bump is refused without a roll
// or a round, and the door still yields to a key.
TEST_F(DoorForcingTest, EighteenHundredFailsOnASevenAndMayNotTryAgain)
{
	become(CreatureClass::FIGHTER, 18, 100);
	mock.dice.set_next_roll(7);

	walk_into_the_door();

	EXPECT_EQ(ctx.messageSystem->get_current_message(), HELD);
	EXPECT_TRUE(ctx.map->has_resisted_forcing(DOOR));
	EXPECT_TRUE(ctx.map->is_door_locked(DOOR));

	// The second bump.
	mock.dice.set_next_roll(UNREAD_D20);
	const int timeBefore = ctx.gameState->get_time();
	walk_into_the_door();

	EXPECT_EQ(ctx.messageSystem->get_current_message(), ALREADY_FAILED);
	expect_refused_without_a_roll(timeBefore);

	// The book limits forcing, not keys.
	Player& player = ctx.player_concrete();
	ASSERT_TRUE(InventoryOperations::add_item(player.inventoryData, ItemCreator::create("dungeon_key", START, ctx)).has_value());
	walk_into_the_door();

	EXPECT_EQ(ctx.map->get_tile_type(DOOR), TileType::OPEN_DOOR) << "a key did not open a door that resisted forcing";
}

// The table decides, not the class: a cleric of Strength 19, 16(8), opens on an 8.
TEST_F(DoorForcingTest, AClericOfStrengthNineteenOpensOnAnEight)
{
	become(CreatureClass::CLERIC, 19, 0);
	mock.dice.set_next_roll(8);

	walk_into_the_door();

	EXPECT_EQ(ctx.map->get_tile_type(DOOR), TileType::OPEN_DOOR);
	EXPECT_EQ(ctx.messageSystem->get_current_message(), FORCED);
}

// end of file: DoorForcingTest.cpp
