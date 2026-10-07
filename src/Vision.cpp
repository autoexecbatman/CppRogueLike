#include <cassert>

#include "Creature.h"
#include "InventoryOperations.h"
#include "Map.h"
#include "Vision.h"

namespace Vision
{

int sight_radius(const Creature& viewer) noexcept
{
	// Asked of the pack every call, so the radius cannot drift from what is carried.
	const int radius = InventoryOperations::is_carrying(viewer.inventoryData, TORCH_ITEM_KEY)
		? TORCH_SIGHT_RADIUS
		: FOV_RADIUS;

	// A light source widens what a creature sees or it is not a light source. This
	// catches a future one whose constant was written below the unlit radius.
	assert(radius >= FOV_RADIUS && "sight_radius: a light source cannot narrow what a creature sees");

	return radius;
}

} // namespace Vision
