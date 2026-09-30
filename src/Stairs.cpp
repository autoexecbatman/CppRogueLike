#include "Stairs.h"
#include "Actor.h"
#include "Colors.h"
#include "Vector2D.h"

Stairs::Stairs(Vector2D position)
	: Actor(position, ActorData{ TileRef{}, "stairs", ColorPairId::WHITE_BLACK }) {}
