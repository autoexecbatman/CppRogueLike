// file: AwarenessTest.cpp
// Verifies that awareness is refreshed by sight, decays over a fixed memory, and
// is never granted by an invisible player. Awareness replaced a tracking counter
// that four Ai classes each maintained by hand, two of which ignored invisibility.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/ActorTypes/Player.h"
#include "src/Map/Map.h"
#include "src/Utils/Vector2D.h"
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

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 4, 5 }) };
	Creature monster{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "goblin", 0 } };
};

// A creature starts with no knowledge of the player.
TEST_F(AwarenessTest, StartsUnaware)
{
	EXPECT_FALSE(monster.is_aware());
}

// Seeing the player grants awareness.
TEST_F(AwarenessTest, SightGrantsAwareness)
{
	make_visible();

	monster.update_awareness(ctx);

	EXPECT_TRUE(monster.is_aware());
}

// Awareness persists for a fixed memory after sight is lost, then expires.
TEST_F(AwarenessTest, AwarenessDecaysOverMemoryThenExpires)
{
	make_visible();
	monster.update_awareness(ctx);

	make_hidden();
	for (int turn = 0; turn < AWARENESS_TURNS - 1; ++turn)
	{
		monster.update_awareness(ctx);
		EXPECT_TRUE(monster.is_aware()) << "still tracking on turn " << turn;
	}

	monster.update_awareness(ctx);

	EXPECT_FALSE(monster.is_aware());
}

// An invisible player is never seen, whatever the line of sight says.
TEST_F(AwarenessTest, InvisiblePlayerGrantsNoAwareness)
{
	make_visible();
	player->add_state(ActorState::IS_INVISIBLE);

	monster.update_awareness(ctx);

	EXPECT_FALSE(monster.is_aware());
}

// A creature that lost the player long ago still notices them again at once,
// and the restored memory is full rather than partial.
//
// The decay floor in update_awareness is deliberately not asserted here: it
// cannot be observed through is_aware(), which reads the same at zero and below.
TEST_F(AwarenessTest, RegainsFullMemoryAfterLongAbsence)
{
	make_hidden();
	for (int turn = 0; turn < AWARENESS_TURNS * 5; ++turn)
	{
		monster.update_awareness(ctx);
	}
	ASSERT_FALSE(monster.is_aware());

	make_visible();
	monster.update_awareness(ctx);

	// Hiding again must buy the full memory, not a leftover fraction of it.
	make_hidden();
	for (int turn = 0; turn < AWARENESS_TURNS - 1; ++turn)
	{
		monster.update_awareness(ctx);
		EXPECT_TRUE(monster.is_aware()) << "still tracking on turn " << turn;
	}
}
