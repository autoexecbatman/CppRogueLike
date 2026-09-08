#include "../Colors/Colors.h"
#include "../Utils/Vector2D.h"
#include "Actor.h"
#include "Stairs.h"

Stairs::Stairs(Vector2D position)
	: Actor(position, ActorData{ TileRef{}, "stairs", WHITE_BLACK_PAIR }) {}
