// file: GameContextWiringTest.cpp
// Integration: GameContext borrows one handle - the unique_ptr that owns the
// player - and derives both views from it. These tests cross the real Game ->
// GameContext boundary with no mocks, and pin that replacing the player is seen
// through the context without anything being refreshed. The two-handle version
// this replaced needed a second assignment after every replacement, and a site
// that forgot it crashed inside Creature::die.
#include <memory>

#include <gtest/gtest.h>

#include "../../src/ActorTypes/Player.h"
#include "../../src/Core/GameContext.h"
#include "../../src/Game.h"

namespace
{

// Before a game starts Game owns no Player, and the creature view says so
// rather than dereferencing an empty owner.
TEST(GameContextWiring, ContextFromFreshGame_HasNoPlayer)
{
	Game game;

	const GameContext ctx = game.context();

	EXPECT_NE(ctx.playerOwner, nullptr);
	EXPECT_EQ(ctx.player(), nullptr);
}

// With a Player owned, both views name that object.
TEST(GameContextWiring, ContextWithPlayer_BothViewsNameIt)
{
	Game game;
	game.player = std::make_unique<Player>(Vector2D{ 0, 0 });

	const GameContext ctx = game.context();

	EXPECT_EQ(ctx.player(), game.player.get());
	EXPECT_EQ(&ctx.player_concrete(), game.player.get());
}

// The case the old two-handle design got wrong: replacing the player through
// the owner is visible immediately, because no copy of the pointer is stored.
TEST(GameContextWiring, ReplacingThePlayer_NeedsNoRefresh)
{
	Game game;
	game.player = std::make_unique<Player>(Vector2D{ 0, 0 });
	const GameContext ctx = game.context();

	*ctx.playerOwner = std::make_unique<Player>(Vector2D{ 1, 1 });

	EXPECT_EQ(ctx.player(), game.player.get());
	EXPECT_EQ(ctx.player()->position.x, 1);
}

} // namespace
