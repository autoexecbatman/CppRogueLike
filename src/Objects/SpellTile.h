#pragma once

#include "../Actor/TileFeature.h"

// file: SpellTile.h
//
// A tile effect a spell left behind: a web, and the walls and clouds that
// follow it.
//
// Spell tiles and traps are both things standing on a floor tile that act when
// a creature enters, which is what TileFeature holds. Everything past that
// differs: a trap is dungeon furniture that hides, triggers once and can be
// disarmed, while a spell tile is cast by somebody and answers to the spell
// that made it. They live in separate containers so neither has to carry the
// other's questions.
//
// This class holds nothing yet. Web is the only spell tile written, and it has
// no state the next one is known to share - a duration, a caster, a dissipation
// rule all wait for a second implementation to say what they should be. What it
// does now is give the container a type, which is what keeps a trap out of it.
//
// Usage -- adding a spell tile to the level:
//
//   auto web = std::make_unique<Web>(position, strength, tileConfig);
//   ctx.spellTiles->push_back(std::move(web));
class SpellTile : public TileFeature
{
public:
	SpellTile(Vector2D position, ActorData data)
		: TileFeature(position, data) {}
};
