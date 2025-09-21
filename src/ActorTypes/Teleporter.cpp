#include "Teleporter.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include "../Map/Map.h"

//==TELEPORTER==
Teleporter::Teleporter() {}

bool Teleporter::use(Item& owner, Creature& wearer)
{
	Vector2D teleportLocation = find_valid_teleport_location();
	
	if (teleportLocation.x != -1 && teleportLocation.y != -1)
	{
		wearer.position = teleportLocation;
		
		// Recalculate FOV from new position
		game.map.compute_fov();
		
		game.message(COLOR_BLUE, "You feel disoriented as the world shifts around you!", true);
		game.message(COLOR_WHITE, "You have been teleported to a new location.", true);
		
		return Pickable::use(owner, wearer);
	}
	else
	{
		game.message(COLOR_RED, "The teleportation magic fizzles out - no safe location found!", true);
		return false;
	}
}

Vector2D Teleporter::find_valid_teleport_location()
{
	// Try up to 50 times to find a valid location
	for (int attempts = 0; attempts < 50; attempts++)
	{
		int x = game.d.roll(2, MAP_WIDTH - 2);
		int y = game.d.roll(2, MAP_HEIGHT - 2);
		
		// Check if the tile is a floor and not occupied by any creature
		if (game.map.get_tile_type(Vector2D{y, x}) == TileType::FLOOR)
		{
			// Check if any creature is at this position
			bool occupied = false;
			for (const auto& creature : game.creatures)
			{
				if (creature && creature->position.x == x && creature->position.y == y)
				{
					occupied = true;
					break;
				}
			}
			
			// Check if player is at this position
			if (!occupied && game.player && (game.player->position.x != x || game.player->position.y != y))
			{
				return Vector2D{y, x};
			}
		}
	}
	
	// If no valid location found, return invalid coordinates
	return Vector2D(-1, -1);
}

void Teleporter::load(const json& j)
{
	// No additional data to load for basic teleporter
}

void Teleporter::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::SCROLL_TELEPORT);
}

Pickable::PickableType Teleporter::get_type() const
{
	return PickableType::SCROLL_TELEPORT;
}
