#pragma once

#include "Actor.h"

class Creature;
struct GameContext;

class Object : public Actor
{
public:
	Object(Vector2D position, ActorData data)
		: Actor(position, data) {};

	// Override in subclasses that block or affect movement through their tile.
	// Returns true when the effect was applied and the move should be interrupted.
	virtual bool apply_movement_effect(Creature& creature, GameContext& ctx) { return false; }
};
