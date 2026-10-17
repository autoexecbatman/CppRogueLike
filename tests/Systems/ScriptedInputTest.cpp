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

// end of file: ScriptedInputTest.cpp
