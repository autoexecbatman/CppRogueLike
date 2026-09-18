#pragma once

#include "Actor.h"

// file: TileFeature.h
//
// What every thing standing on a floor tile shares: a position, a lifetime,
// and one act when a creature enters. Items and stairs are not features -- they
// occupy a tile without acting, and derive from Actor directly.
//
// Two systems build on this and they live in separate containers. Traps are
// dungeon furniture: hidden, triggered once, disarmable, held in Game::traps.
// Spell tiles are what a spell left behind, held in Game::spellTiles under
// SpellTile. Nothing has to ask which one it is holding, because nothing holds
// both.
//
// What belongs here is only what is true of both. Disarming is not: it lives on
// Trap, and the disarm code reaches a Trap directly rather than asking every
// feature whether it happens to be one.
//
// Usage -- deciding whether a move may proceed:
//
//   bool blocked = false;
//   const auto enter_all = [&](auto& features)
//   {
//       for (const auto& feature : features)
//       {
//           if (blocked || feature->position != destination) { continue; }
//           if (feature->on_creature_enter(creature, ctx) == EntryResult::BLOCKED)
//           {
//               blocked = true;                              // move is stopped
//           }
//       }
//   };
//   enter_all(*ctx.traps);                                   // both containers
//   enter_all(*ctx.spellTiles);
//   if (!blocked) { move(destination); }

class Creature;
struct GameContext;

// What a tile feature did to a creature that entered its tile.
//
// BLOCKED implies the creature was acted on, so ask is_affected() rather than
// comparing against UNAFFECTED at a call site.
enum class EntryResult
{
	UNAFFECTED, // the feature did nothing -- a hidden trap that went unnoticed
	AFFECTED, // the feature acted and the creature may still enter
	BLOCKED, // the feature acted and the creature's move is stopped
};

// What a disarm attempt did. Most features cannot be disarmed at all, which is
// a fact about them rather than a missing implementation.
enum class DisarmResult
{
	NOT_DISARMABLE, // this kind of feature has nothing to disarm -- a web
	NOT_VISIBLE, // there is something there, but the creature has not found it
	ALREADY_DISARMED, // disarmed earlier; nothing left to do
	DISARMED, // the attempt succeeded
	TRIGGERED, // the attempt failed and set the feature off
};

class TileFeature : public Actor
{
private:
	bool destroyed{ false };

public:
	TileFeature(Vector2D position, ActorData data)
		: Actor(position, data) {};

	// Marks this feature for removal at the next turn boundary. A feature may
	// destroy itself from inside on_creature_enter, so the entry stays alive and
	// owned until GameLoopCoordinator sweeps it.
	//
	// Example:
	//   web->mark_destroyed();
	//   web->is_destroyed(); // -> true, and the web still renders this turn
	void mark_destroyed() { destroyed = true; }

	[[nodiscard]] bool is_destroyed() const { return destroyed; }

	// Acts on a creature entering this feature's tile, once per entry, before
	// the move is committed. Implementors roll their own saving throws here, so
	// the call changes state and must not be made twice for one entry.
	virtual EntryResult on_creature_enter(Creature& creature, GameContext& ctx) = 0;

};
