#include <curses.h>

#include "Teleporter.h"
#include "../Colors/Colors.h"
#include "../Map/Map.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../Random/RandomDice.h"
#include "../ActorTypes/Player.h"

//==TELEPORTER==
Teleporter::Teleporter() {}

bool Teleporter::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	Vector2D teleportLocation = find_valid_teleport_location(ctx);

	if (teleportLocation.x != -1 && teleportLocation.y != -1)
	{
		wearer.position = teleportLocation;

		// Recalculate FOV from new position
		ctx.map->compute_fov(ctx);

		ctx.message_system->message(COLOR_BLUE, "You feel disoriented as the world shifts around you!", true);
		ctx.message_system->message(COLOR_WHITE, "You have been teleported to a new location.", true);

		return Pickable::use(owner, wearer, ctx);
	}
	else
	{
		ctx.message_system->message(COLOR_RED, "The teleportation magic fizzles out - no safe location found!", true);
		return false;
	}
}

Vector2D Teleporter::find_valid_teleport_location(GameContext& ctx)
{
	// Try up to 50 times to find a valid location
	for (int attempts = 0; attempts < 50; attempts++)
	{
		int x = ctx.dice_roller->roll(2, MAP_WIDTH - 2);
		int y = ctx.dice_roller->roll(2, MAP_HEIGHT - 2);

		// Check if the tile is a floor and not occupied by any creature
		if (ctx.map->get_tile_type(Vector2D{y, x}) == TileType::FLOOR)
		{
			// Check if any creature is at this position
			bool occupied = false;
			for (const auto& creature : *ctx.creatures)
			{
				if (creature && creature->position.x == x && creature->position.y == y)
				{
					occupied = true;
					break;
				}
			}

			// Check if player is at this position
			if (!occupied && ctx.player && (ctx.player->position.x != x || ctx.player->position.y != y))
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
