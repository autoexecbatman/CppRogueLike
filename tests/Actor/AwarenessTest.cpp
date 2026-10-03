// file: AwarenessTest.cpp
// Verifies that awareness is refreshed by sight, decays over a fixed memory, and
// is never granted by an invisible player. Awareness replaced a tracking counter
// that four Ai classes each maintained by hand, two of which ignored invisibility.

#include <gtest/gtest.h>

#include "src/Colors.h"
#include "src/Creature.h"
#include "src/Map.h"
#include "src/Player.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

class AwarenessTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);

		map.init_tiles();

		// Carve an open corridor so line of sight is a property of distance
		// rather than of walls.
		for (int col = 1; col < 19; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR, 1.0);
		}
	}

	// Visibility is computed from the player, so the test moves the player
	// rather than poking the creature's state.
	void make_visible()
	{
		player->position = Vector2D{ 4, 5 };
		map.compute_fov(ctx);
	}

	void make_hidden()
	{
		player->position = Vector2D{ 18, 5 };
		map.compute_fov(ctx);
	}

	// Where the clock stands, and moving it on. A memory ends at a reading rather than
	// after a number of calls, so passing time is what these cases do between checks.
	[[nodiscard]] int now() const { return ctx.gameState->get_time(); }

	void let_rounds_pass(int count) { ctx.gameState->advance_clock(TIME_UNITS_PER_ROUND * count); }

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 4, 5 }) };
	Creature monster{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK } };
};

// A creature starts with no knowledge of the player.
TEST_F(AwarenessTest, StartsUnaware)
{
	EXPECT_FALSE(monster.is_aware(now()));
}

// Seeing the player grants awareness.
TEST_F(AwarenessTest, SightGrantsAwareness)
{
	make_visible();

	monster.update_awareness(ctx);

	EXPECT_TRUE(monster.is_aware(now()));
}

// Awareness holds for its rounds after sight is lost, then ends. Nothing runs it down:
// the clock reaching the reading is the whole of it, so the creature is not updated at
// all between losing sight and forgetting.
TEST_F(AwarenessTest, AwarenessHoldsForItsRoundsThenEnds)
{
	make_visible();
	monster.update_awareness(ctx);

	make_hidden();
	for (int round = 1; round < AWARENESS_ROUNDS; ++round)
	{
		let_rounds_pass(1);
		EXPECT_TRUE(monster.is_aware(now())) << "still tracking in round " << round;
	}

	let_rounds_pass(1);

	EXPECT_FALSE(monster.is_aware(now()));
}

// A memory lasts the rounds it names whoever holds it. A quickling's update runs twice
// a round, and under a counter each run took a turn off its memory, so it lost the
// player in half the rounds an ordinary creature did. Both are driven here the number
// of times their pace would drive them.
TEST_F(AwarenessTest, SpeedDoesNotChangeHowLongAMemoryLasts)
{
	Creature quickling{ Vector2D{ 6, 5 }, ActorData{ TileRef{}, "quickling", ColorPairId::WHITE_BLACK } };
	quickling.set_action_delay(TIME_UNITS_PER_ROUND / 2);

	make_visible();
	monster.update_awareness(ctx);
	quickling.update_awareness(ctx);
	quickling.update_awareness(ctx);

	make_hidden();
	for (int round = 1; round < AWARENESS_ROUNDS; ++round)
	{
		ctx.gameState->set_time(TIME_UNITS_PER_ROUND * round);
		monster.update_awareness(ctx);
		quickling.update_awareness(ctx);
		quickling.update_awareness(ctx);

		EXPECT_TRUE(monster.is_aware(now())) << "round " << round;
		EXPECT_TRUE(quickling.is_aware(now()))
			<< "the quickling forgot the player in round " << round << " of " << AWARENESS_ROUNDS;
	}

	ctx.gameState->set_time(TIME_UNITS_PER_ROUND * AWARENESS_ROUNDS);
	monster.update_awareness(ctx);
	quickling.update_awareness(ctx);

	EXPECT_FALSE(monster.is_aware(now()));
	EXPECT_FALSE(quickling.is_aware(now())) << "the memory outstayed the rounds it names";
}

// The memory is three rounds, said in rounds rather than in terms of the constant.
// Every other case in this file is written relative to AWARENESS_ROUNDS and so cannot
// see it move - a mutation sweep doubling it left all of them green. This one exists
// to see it move, which is the only reason it repeats a number the others imply.
TEST_F(AwarenessTest, TheMemoryIsThreeRounds)
{
	make_visible();
	monster.update_awareness(ctx);
	make_hidden();

	let_rounds_pass(2);
	EXPECT_TRUE(monster.is_aware(now())) << "the memory was gone before its third round";

	let_rounds_pass(1);
	EXPECT_FALSE(monster.is_aware(now())) << "the memory outlasted its third round";
}

// An invisible player is never seen, whatever the line of sight says.
TEST_F(AwarenessTest, InvisiblePlayerGrantsNoAwareness)
{
	make_visible();
	player->add_state(ActorState::IS_INVISIBLE);

	monster.update_awareness(ctx);

	EXPECT_FALSE(monster.is_aware(now()));
}

// A creature that lost the player long ago still notices them again at once, and the
// restored memory is full rather than partial. Under a counter this needed a floor to
// stop the memory going arbitrarily negative while the player stayed away; a reading
// the clock has passed is simply past, so there is no floor to get wrong.
TEST_F(AwarenessTest, RegainsFullMemoryAfterLongAbsence)
{
	make_hidden();
	let_rounds_pass(AWARENESS_ROUNDS * 5);
	ASSERT_FALSE(monster.is_aware(now()));

	make_visible();
	monster.update_awareness(ctx);

	// Hiding again must buy the full memory, not a leftover fraction of it.
	make_hidden();
	for (int round = 1; round < AWARENESS_ROUNDS; ++round)
	{
		let_rounds_pass(1);
		EXPECT_TRUE(monster.is_aware(now())) << "still tracking in round " << round;
	}
}
