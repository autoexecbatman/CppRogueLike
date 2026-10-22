// file: OpenLocksTest.cpp
// Picking a locked door, by the Open Locks text (PHB Chapter 3, Thief Abilities, PDF page
// 86): "The amount of time required to pick a lock is 1d10 rounds. A thief can try to pick
// a particular lock only once per experience level. If the attempt fails, the lock is simply
// too difficult for the character until he learns more about picking locks (goes up a
// level)."
//
// Three readings the text leaves open, decided for the owner and recorded in the decisions
// file: a failed pick takes the 1d10 rounds too; a refused pick goes on to forcing in the
// same bump; and "goes up a level" means a level higher than the one the pick failed at.
//
// Each test walks a thief into a locked door with a key queued on InputSystem, down the
// path a pressed key takes in play, with the d10 and the d100 scripted in that order.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=OpenLocksTest.*

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "src/CreatureClass.h"
#include "src/GameContext.h"
#include "src/InputHandler.h"
#include "src/InputSystem.h"
#include "src/LogMessage.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/Player.h"
#include "src/PlayerController.h"
#include "src/PlayerRaceState.h"
#include "src/ThiefSkills.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A row of floor with the thief on it and a locked door one step east.
constexpr int CORRIDOR_ROW = 5;
const Vector2D START{ 4, CORRIDOR_ROW };
const Vector2D DOOR{ 5, CORRIDOR_ROW };

const std::string PICKED{ "You pick the lock." };
const std::string FAILED{ "You fail to pick the lock." };
const std::string TOO_DIFFICULT{ "This lock is too difficult for you until you have learned more." };
const std::string FORCED{ "You force the door open! The crash echoes down the corridor." };
const std::string TOO_WEAK{ "The door is locked. You are not strong enough to force it." };
// A queued value no refused attempt is meant to read, so reading it back proves nothing rolled.
constexpr int UNREAD_ROLL = 13;
// Points put into Open Locks, enough that the chance is above every d10 these tests queue,
// so a d100 read where the d10 belongs would pick the lock and fail the test.
constexpr int OPEN_LOCKS_POINTS = 30;
} // namespace

class OpenLocksTest : public ::testing::Test
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

		Player& thief = ctx.player_concrete();
		thief.set_creature_class(CreatureClass::ROGUE);
		thief.lifespan->race = PlayerRaceState::HUMAN;
		thief.set_dexterity(13);
		thief.set_creature_level(1);
		thief.thiefSkillPoints.at(thief_skill_index(ThiefSkill::OPEN_LOCKS)) = OPEN_LOCKS_POINTS;
		// Strength 17 prints no locked-door chance, so a thief refused a pick cannot force either.
		set_strength(17, 0);
		const std::optional<int> skill = thief.thief_skill(ThiefSkill::OPEN_LOCKS);
		ASSERT_TRUE(skill.has_value());
		openLocksChance = *skill;
		ASSERT_GT(openLocksChance, 10) << "a chance a d10 could reach hides a d100 read in the d10's place";
		// An ordinary pace charges the rounds as they are.
		ASSERT_EQ(thief.get_action_delay(), TIME_UNITS_PER_ROUND);
	}

	void set_strength(int strength, int exceptional)
	{
		Player& thief = ctx.player_concrete();
		thief.set_strength(strength);
		thief.set_exceptional_strength(exceptional);
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

	// A pick that fails, taking the d10's rounds.
	void fail_a_pick(int rounds)
	{
		mock.dice.set_next_roll(rounds);
		mock.dice.set_next_roll(openLocksChance + 1);
		walk_into_the_door();
	}

	// How many stored messages hold the given text, their coloured runs joined.
	size_t times_said(std::string_view wanted) const
	{
		size_t count = 0;
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			std::string whole;
			for (const LogMessage& part : ctx.messageSystem->get_attack_message_at(index))
			{
				whole += part.text;
			}
			if (whole.find(wanted) != std::string::npos)
			{
				++count;
			}
		}
		return count;
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
	int openLocksChance{ 0 };
};

// A failure takes the d10's rounds, as a success does, and the lock remembers the level.
TEST_F(OpenLocksTest, AFailedPickTakesTheDTensRoundsAndIsRemembered)
{
	const int timeBefore = ctx.gameState->get_time();

	fail_a_pick(7);

	EXPECT_EQ(ctx.messageSystem->get_current_message(), FAILED);
	EXPECT_TRUE(ctx.map->is_door_locked(DOOR));
	EXPECT_EQ(ctx.map->lock_pick_failed_at_level(DOOR), 1);
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore + 7 * TIME_UNITS_PER_ROUND);
}

TEST_F(OpenLocksTest, ASuccessfulPickTakesTheDTensRoundsAndOpensTheDoor)
{
	const int timeBefore = ctx.gameState->get_time();
	mock.dice.set_next_roll(4);
	mock.dice.set_next_roll(openLocksChance);

	walk_into_the_door();

	EXPECT_EQ(ctx.messageSystem->get_current_message(), PICKED);
	EXPECT_EQ(ctx.map->get_tile_type(DOOR), TileType::OPEN_DOOR);
	EXPECT_FALSE(ctx.map->is_door_locked(DOOR));
	EXPECT_EQ(ctx.map->lock_pick_failed_at_level(DOOR), 0) << "a success was recorded as a failure";
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore + 4 * TIME_UNITS_PER_ROUND);
}

// Once per level: the second bump is refused without a roll or a round. On a Strength row
// with no locked-door chance the forcing refusal follows it, and still nothing is spent.
TEST_F(OpenLocksTest, ASecondPickAtTheSameLevelIsRefusedWithoutARoll)
{
	fail_a_pick(2);
	mock.dice.set_next_roll(UNREAD_ROLL);
	const int timeBefore = ctx.gameState->get_time();

	walk_into_the_door();

	EXPECT_EQ(times_said(TOO_DIFFICULT), 1u);
	EXPECT_EQ(ctx.messageSystem->get_current_message(), TOO_WEAK);
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore) << "a refused pick spent time";
	EXPECT_TRUE(ctx.map->is_door_locked(DOOR));
	EXPECT_EQ(ctx.player()->position, START);
	EXPECT_EQ(ctx.map->lock_pick_failed_at_level(DOOR), 1);
	EXPECT_EQ(mock.dice.d100(), UNREAD_ROLL) << "a refused pick rolled";
}

// "goes up a level": one level higher, and the lock may be tried again.
TEST_F(OpenLocksTest, AfterALevelUpTheLockMayBeTriedAgain)
{
	fail_a_pick(2);
	ctx.player_concrete().set_creature_level(2);
	mock.dice.set_next_roll(3);
	mock.dice.set_next_roll(1);

	walk_into_the_door();

	EXPECT_EQ(ctx.messageSystem->get_current_message(), PICKED);
	EXPECT_EQ(ctx.map->get_tile_type(DOOR), TileType::OPEN_DOOR);
}

// A second failure, at the higher level, is remembered at that level.
TEST_F(OpenLocksTest, AFailureAfterALevelUpIsRememberedAtTheNewLevel)
{
	fail_a_pick(2);
	ctx.player_concrete().set_creature_level(2);

	fail_a_pick(5);

	EXPECT_EQ(ctx.messageSystem->get_current_message(), FAILED);
	EXPECT_EQ(ctx.map->lock_pick_failed_at_level(DOOR), 2);
}

// Table 1 limits forcing and Open Locks limits picking, so a door that resisted forcing still
// yields to a pick, which is tried first.
TEST_F(OpenLocksTest, ADoorThatResistedForcingStillYieldsToAPick)
{
	ctx.map->mark_resisted_forcing(DOOR);
	const int timeBefore = ctx.gameState->get_time();
	mock.dice.set_next_roll(6);
	mock.dice.set_next_roll(openLocksChance);

	walk_into_the_door();

	EXPECT_EQ(ctx.map->get_tile_type(DOOR), TileType::OPEN_DOOR);
	EXPECT_EQ(ctx.messageSystem->get_current_message(), PICKED);
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore + 6 * TIME_UNITS_PER_ROUND);
}

// A refused pick goes on to forcing in the same bump: Strength 18/00 prints 16(6), so a 6 opens.
TEST_F(OpenLocksTest, ARefusedPickOnAStrongThiefForcesInTheSameBump)
{
	set_strength(18, 100);
	fail_a_pick(2);
	mock.dice.set_next_roll(6);
	const int timeBefore = ctx.gameState->get_time();

	walk_into_the_door();

	EXPECT_EQ(times_said(TOO_DIFFICULT), 1u);
	EXPECT_EQ(ctx.messageSystem->get_current_message(), FORCED);
	EXPECT_EQ(ctx.map->get_tile_type(DOOR), TileType::OPEN_DOOR);
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore + TIME_UNITS_PER_ROUND);
}

// end of file: OpenLocksTest.cpp
