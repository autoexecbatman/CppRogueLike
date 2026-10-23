// file: PlayerConfusionTest.cpp
// A confused player stumbles, by the clock reading the confusion was given, whoever
// confused them - a mimic, or a scroll read at their own tile - and is told once when
// their mind clears.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PlayerConfusionTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>

#include "src/AiMimic.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/InputHandler.h"
#include "src/InputSystem.h"
#include "src/LogMessage.h"
#include "src/Map.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/PlayerController.h"
#include "src/Renderer.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A row of open floor with the player standing on it, room to stumble either way.
constexpr int CORRIDOR_ROW = 5;
const Vector2D START{ 4, CORRIDOR_ROW };
// The stumble's direction roll indexes N, S, W, E, NW, NE, SW, SE; 3 is east.
constexpr int STUMBLE_EAST = 3;
} // namespace

class PlayerConfusionTest : public ::testing::Test
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

	// One frame of input with no key pressed, then the player's turn.
	void take_a_turn()
	{
		ctx.inputHandler->reset_key();
		ctx.inputSystem->poll();
		ctx.inputHandler->key_store();
		ctx.inputHandler->key_listen(*ctx.inputSystem);
		ctx.player_concrete().controller->update(ctx);
	}

	// How many stored messages hold the given text, their coloured runs joined.
	size_t times_said(std::string_view wanted) const
	{
		size_t count = 0;
		for (
			size_t index = 0;
			index < ctx.messageSystem->get_stored_message_count();
			++index)
		{
			std::string whole;
			for (
				const LogMessage& run :
				ctx.messageSystem->get_attack_message_at(index))
			{
				whole += run.text;
			}
			if (whole.find(wanted) != std::string::npos)
			{
				++count;
			}
		}
		return count;
	}

	bool is_confused_now() const
	{
		return ctx.player_concrete().controller->is_confused(
			ctx.gameState->get_time());
	}

	MockGameContext mock{};
	GameContext ctx{ mock.to_game_context() };
};

// Confusing the player leaves them confused for the rounds given, by the clock.
TEST_F(PlayerConfusionTest, ApplyingConfusionToThePlayerMarksThemConfused)
{
	ctx.player()->apply_confusion(3, ctx.gameState->get_time());

	EXPECT_TRUE(is_confused_now());
}

// A confused player whose d2 comes up 1 stumbles in the rolled direction.
TEST_F(PlayerConfusionTest, AConfusedPlayerStumbles)
{
	ctx.player()->apply_confusion(3, ctx.gameState->get_time());
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(STUMBLE_EAST);

	take_a_turn();

	EXPECT_EQ(ctx.player()->position, (Vector2D{ START.x + 1, CORRIDOR_ROW }))
		<< "the confused player did not stumble";
	EXPECT_EQ(times_said("You stumble around in confusion!"), 1u);
}

// A confusion scroll read at the player's own tile confuses them as any other source does.
TEST_F(PlayerConfusionTest, AConfusionScrollReadAtThePlayerConfusesThem)
{
	ASSERT_EQ(
		read_confusion_at(*ctx.player(), START, 3, ctx),
		ScrollReading::SPENT);
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(STUMBLE_EAST);

	take_a_turn();

	EXPECT_TRUE(is_confused_now());
	EXPECT_EQ(ctx.player()->position, (Vector2D{ START.x + 1, CORRIDOR_ROW }))
		<< "the scroll's confusion made nobody stumble";
}

// When the confusion's rounds have passed, the player is told once and is clear-headed.
TEST_F(PlayerConfusionTest, TheMindClearsWhenTheConfusionEnds)
{
	ctx.player()->apply_confusion(1, ctx.gameState->get_time());
	ctx.gameState->advance_clock(TIME_UNITS_PER_ROUND);

	take_a_turn();
	take_a_turn();

	EXPECT_EQ(times_said("Your mind clears."), 1u)
		<< "the clearing was not reported exactly once";
	EXPECT_FALSE(is_confused_now());
}

// A player who was never confused is never told their mind clears.
TEST_F(PlayerConfusionTest, AnUnconfusedPlayerIsNeverToldTheirMindClears)
{
	take_a_turn();
	take_a_turn();

	EXPECT_EQ(times_said("Your mind clears."), 0u);
	EXPECT_FALSE(is_confused_now());
}

// A mimic revealing itself beside the player confuses them when the d20 beats their Wisdom.
TEST_F(PlayerConfusionTest, AMimicRevealingItselfConfusesThePlayer)
{
	ctx.player()->set_wisdom(3);
	Creature mimic{
		Vector2D{ START.x + 1, CORRIDOR_ROW },
		ActorData{ TileRef{}, "chest", ColorPairId::WHITE_BLACK },
	};
	mimic.healthPool = std::make_unique<HealthPool>(10);
	mimic.ai = std::make_unique<AiMimic>();
	mock.dice.set_next_roll(20);

	mimic.ai->update(mimic, ctx);

	EXPECT_TRUE(is_confused_now())
		<< "the mimic's reveal left the player clear-headed";
}
