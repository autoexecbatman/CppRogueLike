// file: PlayerTurn.cpp
#include "PlayerTurn.h"

#include <cassert>

#include "Creature.h"
#include "GameContext.h"

void spend_player_action(GameContext& ctx, int baseCost)
{
	assert(ctx.player() && "spend_player_action: an action was spent with no player");
	assert(ctx.gameState && "spend_player_action: an action was spent with no game state");

	// The clock is the player's own place on it: the game advances when the player
	// acts and at no other time, so keeping a second number for the player would be
	// two answers to one question. Every other creature is measured against this.
	ctx.gameState->advance_clock(scaled_cost(baseCost, ctx.player()->get_speed()));

	ctx.gameState->set_game_status(GameStatus::NEW_TURN);
}

// end of file: PlayerTurn.cpp
