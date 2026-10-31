#include "Stairs.h"
#include "Actor.h"
#include "Colors.h"
#include "TileRef.h"
#include "Vector2D.h"

namespace CppRL
{

Stairs::Stairs(Vector2D position) :
	Actor(position, ActorData{ TileRef{}, "stairs", ColorPairId::WHITE_BLACK })
{
}

} // namespace CppRL
