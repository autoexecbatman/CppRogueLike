// file: GameContext.cpp
//
// The player, reached two ways from one stored handle.
//
// GameContext owns nothing. Game owns the player, and the context borrows the
// handle that owns it - playerOwner. Both views below are computed from that
// one handle, so there is no second copy that could name a different object.
//
// Usage:
//
//   ctx.player()->position                          // as a creature
//   ctx.player_concrete().get_class_display_name()  // as a player
//
//   *ctx.playerOwner = std::make_unique<Player>(pos, blueprint, ctx);
//   // no second handle to refresh: both views follow the assignment
//
// player() is the one nearly every caller wants: 83 of the 93 reads in src/ ask
// for something a monster answers too - position, health, states. It returns
// Creature*, and the upcast happens here rather than at the call site, so those
// callers need no Player.h and the header coupling commit 4b92eb2 removed stays
// removed.

#include <cassert>

#include "../ActorTypes/Player.h"
#include "GameContext.h"

// The player as a creature, or null before one exists.
//
// Safe on a half-built context: a context whose playerOwner is unset, or whose
// owner holds nothing, reports null rather than dereferencing.
//
// Example:
//   GameContext ctx;
//   ctx.player();                   // -> nullptr, no player yet
//
//   ctx.playerOwner = &owned;       // owned holds a Player
//   ctx.player()->position;         // -> the player's position
//
//   *ctx.playerOwner = std::make_unique<Player>(elsewhere, blueprint, ctx);
//   ctx.player()->position;         // -> the new player's position, no refresh
Creature* GameContext::player() const
{
	// Both an unset handle and an empty owner mean there is no player yet.
	if (playerOwner == nullptr)
	{
		return nullptr;
	}
	return playerOwner->get();
}

// The player as itself, for callers that need what only a player has.
//
// Refuses a context with no player: this is for code that runs while a game is
// in progress, where the absence of a player is a defect rather than a state to
// branch on. Callers that can legitimately run before the player exists should
// test player() instead.
//
// Example:
//   ctx.player_concrete().get_class_display_name();   // -> "Fighter"
//   ctx.player_concrete().on_kill_reward(xp, ctx);
//
//   GameContext empty;
//   empty.player_concrete();                          // aborts:
//     Assertion failed: playerOwner != nullptr && playerOwner->get() != nullptr
//       && "GameContext: no player in context"
Player& GameContext::player_concrete() const
{
	assert(playerOwner != nullptr && playerOwner->get() != nullptr
		&& "GameContext: no player in context");
	return **playerOwner;
}
