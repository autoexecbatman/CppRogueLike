// file: PlayerTurn.cpp
#include <cassert>

#include "Creature.h"
#include "GameContext.h"
#include "PlayerTurn.h"

void spend_player_action(GameContext& ctx, int baseCost)
{
	assert(ctx.player() && "spend_player_action: an action was spent with no player");
	assert(ctx.gameState && "spend_player_action: an action was spent with no game state");

	// The clock is the player's place on it: it advances only when the player acts, and every other
	// creature is measured against it.
	ctx.gameState->advance_clock(scaled_cost(baseCost, ctx.player()->get_action_delay()));

	ctx.gameState->set_game_status(GameStatus::NEW_TURN);
}

// end of file: PlayerTurn.cpp
