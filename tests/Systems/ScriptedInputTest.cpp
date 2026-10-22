// file: ScriptedInputTest.cpp
// The scripted input double: keys a test queues on InputSystem come out of poll() in order,
// and reach the player's controller down the same path a pressed key takes in play -
// poll(), then InputHandler::key_listen, then PlayerController::update.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=ScriptedInputTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/GameContext.h"
#include "src/InputHandler.h"
#include "src/InputSystem.h"
#include "src/Map.h"
#include "src/Player.h"
#include "src/PlayerController.h"
#include "src/PlayerRaceState.h"
#include "src/ThiefSkills.h"
#include "src/Trap.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A row of open floor with the player standing on it, room to step either way.
constexpr int CORRIDOR_ROW = 5;
const Vector2D START{ 4, CORRIDOR_ROW };
} // namespace

class ScriptedInputTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		mock.map.init_tiles();
		for (int col = 1; col < 10; ++col)
		{
			mock.map.set_tile(Vector2D{ col, CORRIDOR_ROW }, TileType::FLOOR);
		}
		mock.player = std::make_unique<Player>(START);
	}

	// One frame of the game loop's input step, as GameLoopCoordinator runs it.
	void read_one_frame_of_input()
	{
		ctx.inputHandler->reset_key();
		ctx.inputSystem->poll();
		ctx.inputHandler->key_store();
		ctx.inputHandler->key_listen(*ctx.inputSystem);
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
};

TEST_F(ScriptedInputTest, QueuedKeysComeOutOfPollOldestFirst)
{
	ctx.inputSystem->queue_key(GameKey::RIGHT, 0);
	ctx.inputSystem->queue_key(GameKey::NONE, 'g');

	ctx.inputSystem->poll();
	EXPECT_EQ(ctx.inputSystem->get_key(), GameKey::RIGHT);
	EXPECT_EQ(ctx.inputSystem->get_char_input(), 0);

	ctx.inputSystem->poll();
	EXPECT_EQ(ctx.inputSystem->get_key(), GameKey::NONE);
	EXPECT_EQ(ctx.inputSystem->get_char_input(), 'g');
}

TEST_F(ScriptedInputTest, AFrameAfterTheLastQueuedKeyReadsNoKey)
{
	ctx.inputSystem->queue_key(GameKey::ESCAPE, 0);

	ctx.inputSystem->poll();
	ctx.inputSystem->poll();

	EXPECT_EQ(ctx.inputSystem->get_key(), GameKey::NONE);
	EXPECT_EQ(ctx.inputSystem->get_char_input(), 0);
}

TEST_F(ScriptedInputTest, AQueuedRightArrowWalksThePlayerOneStepEast)
{
	ctx.inputSystem->queue_key(GameKey::RIGHT, 0);

	read_one_frame_of_input();
	ctx.player_concrete().controller->update(ctx);

	EXPECT_EQ(ctx.player()->position, (Vector2D{ START.x + 1, CORRIDOR_ROW }));
}

TEST_F(ScriptedInputTest, AQueuedLeftArrowWalksThePlayerOneStepWest)
{
	ctx.inputSystem->queue_key(GameKey::LEFT, 0);

	read_one_frame_of_input();
	ctx.player_concrete().controller->update(ctx);

	EXPECT_EQ(ctx.player()->position, (Vector2D{ START.x - 1, CORRIDOR_ROW }));
}

// The disarm key, then a direction, reaches a found trap on the floor. A trap is only
// ever placed on floor, so a disarm the door check refused could never reach one.
TEST_F(ScriptedInputTest, TheDisarmKeyReachesAFoundTrapOnTheFloor)
{
	Player& thief = ctx.player_concrete();
	thief.set_creature_class(CreatureClass::ROGUE);
	thief.lifespan->race = PlayerRaceState::HUMAN;
	thief.set_dexterity(13);
	thief.set_creature_level(1);
	thief.thiefSkillPoints.at(thief_skill_index(ThiefSkill::FIND_REMOVE_TRAPS)) = 40;
	mock.traps.push_back(std::make_unique<Trap>(Vector2D{ START.x + 1, CORRIDOR_ROW }, TrapType::DART, mock.tile_config));
	Trap& dart = *mock.traps.back();
	mock.dice.set_next_roll(1);
	ASSERT_EQ(dart.search(thief, ctx), TrapSearchResult::FOUND);

	// The disarm key, then east; the d100 is a certain success and the 1d10 shows 7. An
	// ordinary pace charges the rounds as they are.
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(7);
	ASSERT_EQ(thief.get_action_delay(), TIME_UNITS_PER_ROUND);
	const int timeBefore = ctx.gameState->get_time();
	ctx.inputSystem->queue_key(GameKey::S, 's');
	read_one_frame_of_input();
	ctx.player_concrete().controller->update(ctx);
	ctx.inputSystem->queue_key(GameKey::RIGHT, 0);
	read_one_frame_of_input();
	ctx.player_concrete().controller->update(ctx);

	EXPECT_EQ(dart.get_state(), TrapState::DISARMED);
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore + 7 * TIME_UNITS_PER_ROUND) << "the attempt did not take its seven rounds";
	EXPECT_EQ(ctx.player()->position, START) << "the direction walked the thief instead of aiming the disarm";
}

// A frame carrying a game key advances the game: the loop runs its update on it.
TEST_F(ScriptedInputTest, AFrameWithAGameKeyRunsTheUpdate)
{
	ctx.inputSystem->queue_key(GameKey::RIGHT, 0);
	read_one_frame_of_input();

	EXPECT_TRUE(ctx.inputHandler->is_logic_frame());
}

// So does a frame carrying a typed character.
TEST_F(ScriptedInputTest, AFrameWithACharacterRunsTheUpdate)
{
	ctx.inputSystem->queue_key(GameKey::NONE, 'g');
	read_one_frame_of_input();

	EXPECT_TRUE(ctx.inputHandler->is_logic_frame());
}

// A frame with no key only animates; the game waits.
TEST_F(ScriptedInputTest, AFrameWithNoKeyLeavesTheGameWaiting)
{
	read_one_frame_of_input();

	EXPECT_FALSE(ctx.inputHandler->is_logic_frame());
}

// end of file: ScriptedInputTest.cpp
