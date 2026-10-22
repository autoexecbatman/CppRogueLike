// file: PlayerTurn.cpp
#include <cassert>

#include "Creature.h"
#include "GameContext.h"
#include "PlayerTurn.h"

namespace
{
// Moves the player's place on the clock by this many time units and opens the turn.
//
// Example:
//   advance_player_and_open_turn(ctx, 60);   // clock 5000 -> 5060, status NEW_TURN
void advance_player_and_open_turn(GameContext& ctx, int timeUnits)
{
	// The clock is the player's place on it: it advances only when the player acts, and every other
	// creature is measured against it.
	ctx.gameState->advance_clock(timeUnits);

	ctx.gameState->set_game_status(GameStatus::NEW_TURN);
}
} // namespace

void spend_player_action(GameContext& ctx, int baseCost)
{
	assert(ctx.player() && "spend_player_action: an action was spent with no player");
	assert(ctx.gameState && "spend_player_action: an action was spent with no game state");

	advance_player_and_open_turn(ctx, scaled_cost(baseCost, ctx.player()->get_action_delay()));
}

void spend_player_spellcasting(GameContext& ctx)
{
	assert(ctx.player() && "spend_player_spellcasting: a spell was cast with no player");
	assert(ctx.gameState && "spend_player_spellcasting: a spell was cast with no game state");

	advance_player_and_open_turn(ctx, scaled_cost(TIME_UNITS_PER_ROUND, ctx.player()->get_unhasted_action_delay()));
}

// end of file: PlayerTurn.cpp
