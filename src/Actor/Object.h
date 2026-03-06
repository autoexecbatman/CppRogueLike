#pragma once

#include "Actor.h"

class Object : public Actor
{
public:
	Object(Vector2D position, ActorData data)
		: Actor(position, data) {};
};
