#pragma once

#include "Vector2D.h"
#include "Actor.h"
#include "Creature.h"

class NPC : public Creature
{
public:
	NPC(Vector2D position, ActorData data)
		: Creature(position, data) {};
};