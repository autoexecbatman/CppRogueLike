#pragma once

#include "Actor.h"
#include "Creature.h"
#include "Vector2D.h"

class NPC : public Creature
{
public:
	NPC(Vector2D position, ActorData data)
		: Creature(position, data) {};
};