// file: PlayerRestTest.cpp
//
// What stops a player resting, and what does not.
//
// Resting trades a turn's worth of hunger for a fifth of the player's maximum hit
// points. It refuses three ways and each refusal says which one it was, so these
// tests assert on the message rather than on the bool - "it returned false" cannot
// tell a creature standing too close from an empty stomach.
//
// The rule worth pinning is the distance one. It is Chebyshev, so the danger zone is
// a square: a hostile on the diagonal is as near as one straight ahead, and a test
// that only ever places creatures on an axis cannot see the difference.
//
// One test states a distance as a number instead of against REST_DANGER_RADIUS. A
// test written relative to the constant moves when the constant moves, so it cannot
// see the radius itself change - a mutation sweep that widened the zone by one tile
// survived every test here until one of them was written in absolute tiles.
//
// What is not here, and why: the path where resting succeeds. It ends in
// animate_resting, which reads ctx.floatingText and ctx.renderingManager, and
// MockGameContext provides neither - see issues/0141. Every test below therefore
// leaves the player starving, so the scan runs to the end and the hunger refusal is
// what reports that nothing in the creature list blocked it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PlayerRestTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "src/Creature.h"
#include "src/HealthPool.h"
#include "src/HungerSystem.h"
#include "src/LogMessage.h"
#include "src/MessageSystem.h"
#include "src/Player.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr Vector2D THE_PLAYERS_TILE{ 10, 10 };
// Past STARVING_THRESHOLD, so the scan that finds nothing ends in the hunger refusal.
constexpr int ENOUGH_HUNGER_TO_STARVE = 950;
} // namespace

class PlayerRestTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.creatures = &creatures;
		ctx.hungerSystem = &hunger;

		player = std::make_unique<Player>(THE_PLAYERS_TILE);
		player->healthPool = std::make_unique<HealthPool>(40);
		// Resting refuses outright at full health, so every test starts hurt.
		player->healthPool->set_hp(10);

		hunger.increase_hunger(ctx, ENOUGH_HUNGER_TO_STARVE);
	}

	// Puts a creature on a tile and hands it back, so a test can kill it or change
	// how it feels about the player.
	Creature& standing_at(Vector2D where, Attitude attitude)
	{
		auto creature = std::make_unique<Creature>(
			where, ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK });
		creature->healthPool = std::make_unique<HealthPool>(8);
		creature->set_attitude(attitude);
		Creature* placed = creature.get();
		creatures.push_back(std::move(creature));
		return *placed;
	}

	// Whether any stored message holds the given text. The system keeps each message
	// as a vector of coloured runs, so the words have to be joined before searching.
	bool said_anything_about(std::string_view wanted) const
	{
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			std::string whole;
			for (const LogMessage& run : ctx.messageSystem->get_attack_message_at(index))
			{
				whole += run.text;
			}
			if (whole.find(wanted) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	MockGameContext mock{};
	GameContext ctx{};
	HungerSystem hunger{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::unique_ptr<Player> player;
};

// Nothing to heal, so the turn is not spent.
TEST_F(PlayerRestTest, AtFullHealthRestingIsRefused)
{
	player->healthPool->set_hp(player->get_max_hp());

	EXPECT_FALSE(player->rest(ctx));
	EXPECT_TRUE(said_anything_about("already at full health")) << "the refusal did not say the player was unhurt";
}

// The edge of the danger zone is inside it.
TEST_F(PlayerRestTest, AHostileAtTheEdgeOfTheRadiusBlocksTheRest)
{
	standing_at(Vector2D{ THE_PLAYERS_TILE.x + REST_DANGER_RADIUS, THE_PLAYERS_TILE.y }, Attitude::HOSTILE);

	EXPECT_FALSE(player->rest(ctx));
	EXPECT_TRUE(said_anything_about("enemies nearby")) << "a hostile at the edge of the radius did not stop the rest";
}

// Six tiles is far enough away to rest. This one is written in absolute tiles rather
// than against REST_DANGER_RADIUS, deliberately: it is the only test here that can see
// the radius itself change, because every other one moves its creature along with the
// constant. The hunger refusal is how it knows the creature was not what stopped the
// rest.
TEST_F(PlayerRestTest, AHostileSixTilesAwayDoesNotBlockTheRest)
{
	standing_at(Vector2D{ THE_PLAYERS_TILE.x + 6, THE_PLAYERS_TILE.y }, Attitude::HOSTILE);

	EXPECT_FALSE(player->rest(ctx));
	EXPECT_FALSE(said_anything_about("enemies nearby")) << "a hostile outside the radius stopped the rest";
	EXPECT_TRUE(said_anything_about("too hungry")) << "the scan did not run to the end";
}

// The zone is a square, not a circle: this creature is REST_DANGER_RADIUS away on
// both axes, which straight-line distance would put well outside it.
TEST_F(PlayerRestTest, AHostileOnTheDiagonalBlocksAtTheSameRange)
{
	standing_at(
		Vector2D{ THE_PLAYERS_TILE.x + REST_DANGER_RADIUS, THE_PLAYERS_TILE.y + REST_DANGER_RADIUS },
		Attitude::HOSTILE);

	EXPECT_FALSE(player->rest(ctx));
	EXPECT_TRUE(said_anything_about("enemies nearby")) << "the danger zone was measured as a circle rather than a square";
}

// A corpse is not a threat. It stays in the list until the loop sweeps it.
TEST_F(PlayerRestTest, ADeadHostileDoesNotBlockTheRest)
{
	Creature& orc = standing_at(Vector2D{ THE_PLAYERS_TILE.x + 1, THE_PLAYERS_TILE.y }, Attitude::HOSTILE);
	orc.healthPool->set_hp(0);

	EXPECT_FALSE(player->rest(ctx));
	EXPECT_FALSE(said_anything_about("enemies nearby")) << "a dead creature was counted as a threat";
	EXPECT_TRUE(said_anything_about("too hungry")) << "the scan did not run to the end";
}

// This is what lets a player rest inside a shop with the shopkeeper beside them.
TEST_F(PlayerRestTest, APeacefulCreatureStandingAdjacentDoesNotBlockTheRest)
{
	standing_at(Vector2D{ THE_PLAYERS_TILE.x + 1, THE_PLAYERS_TILE.y }, Attitude::PEACEFUL);

	EXPECT_FALSE(player->rest(ctx));
	EXPECT_FALSE(said_anything_about("enemies nearby")) << "a peaceful creature was counted as a threat";
	EXPECT_TRUE(said_anything_about("too hungry")) << "the scan did not run to the end";
}
