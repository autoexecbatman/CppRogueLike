#pragma once

// file: PlayerTurn.h
//
// The one place that says the player has spent an action. Before this existed, 33
// sites across eight files each set the game status to NEW_TURN by hand, which said
// "a turn was used" and could say nothing about how much of one.
//
// An action advances the player's place on the shared clock by what it cost, priced
// against the player's own action delay, and then opens the turn so the creatures and the round
// upkeep follow. Routing every site through here is what makes an action's cost a
// thing the game can vary: a quick step and a full swing are the same event to a
// status enum and different numbers on a clock.
//
// Usage - an ordinary action, which is what every caller passes today:
//
//   spend_player_action(ctx, TIME_UNITS_PER_ROUND);
//
// A cheaper action, once something has a reason to be cheaper:
//
//   spend_player_action(ctx, TIME_UNITS_PER_ROUND / 2);
//
// The cost is always given explicitly. There is no default, because a default is how
// every action ends up costing the same thing again.

#include "TurnSchedule.h"

struct GameContext;

// Records that the player spent an action of this base cost, in time units, and opens
// the turn. TIME_UNITS_PER_ROUND is one ordinary action; the player's own action delay
// prices what is actually charged.
//
// Refuses nothing and returns nothing: by the time a caller reaches here the action
// has happened, and this is the bookkeeping. It asserts a player and a game state,
// both of which a running game always has.
void spend_player_action(GameContext& ctx, int baseCost);
