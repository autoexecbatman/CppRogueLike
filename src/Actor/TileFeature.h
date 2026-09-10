#pragma once

#include "Actor.h"

// file: TileFeature.h
//
// A dungeon feature bound to a tile that acts on creatures entering it --
// traps and spider webs today, and the persistent magical areas that will join
// them. Items and stairs are not features: they occupy a tile without acting,
// and derive from Actor directly.
//
// Every feature lives in Game::tileFeatures and is reached polymorphically from
// there. The one operation is on_creature_enter, called before a move is
// committed so the feature can stop it.
//
// Usage -- deciding whether a move may proceed:
//
//   bool blocked = false;
//   for (const auto& feature : *ctx.tileFeatures)            // features on this level
//   {
//       if (feature->position != destination) { continue; }  // only the entered tile
//       if (feature->on_creature_enter(creature, ctx) == EntryResult::BLOCKED)
//       {
//           blocked = true;                                  // move is stopped
//           break;
//       }
//   }
//   if (!blocked) { move(destination); }                     // nothing blocked it

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

// Which kind of feature this is, for callers that need to find one kind among
// the rest. Identity lives here rather than in ActorData::name, which is a
// display string and free to change.
enum class FeatureKind
{
	TRAP,
	WEB,
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
	FeatureKind kind{ FeatureKind::TRAP };
	bool destroyed{ false };

public:
	TileFeature(Vector2D position, ActorData data, FeatureKind featureKind)
		: Actor(position, data),
		kind(featureKind) {};

	// Which kind of feature this is.
	//
	// Example:
	//   web->get_kind() == FeatureKind::WEB; // -> true
	[[nodiscard]] FeatureKind get_kind() const { return kind; }

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

	// Attempts to disarm this feature. Every feature answers, including the
	// ones that refuse.
	//
	// Example:
	//   web->attempt_disarm(player, ctx); // -> DisarmResult::NOT_DISARMABLE
	virtual DisarmResult attempt_disarm(Creature& creature, GameContext& ctx) = 0;
};
