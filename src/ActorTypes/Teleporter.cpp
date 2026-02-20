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

		ctx.message_system->message(BLUE_BLACK_PAIR, "You feel disoriented as the world shifts around you!", true);
		ctx.message_system->message(WHITE_BLACK_PAIR, "You have been teleported to a new location.", true);

		return Pickable::use(owner, wearer, ctx);
	}
	else
	{
		ctx.message_system->message(RED_BLACK_PAIR, "The teleportation magic fizzles out - no safe location found!", true);
		return false;
	}
}

Vector2D Teleporter::find_valid_teleport_location(GameContext& ctx)
{
	const auto is_position_free = [&](int x, int y) -> bool
	{
		for (const auto& creature : *ctx.creatures)
		{
			if (creature && creature->position.x == x && creature->position.y == y)
				return false;
		}
		if (ctx.player && ctx.player->position.x == x && ctx.player->position.y == y)
			return false;
		return true;
	};

	for (int attempts = 0; attempts < 50; attempts++)
	{
		const int x = ctx.dice->roll(2, ctx.map->get_width() - 2);
		const int y = ctx.dice->roll(2, ctx.map->get_height() - 2);

		if (ctx.map->get_tile_type(Vector2D{x, y}) == TileType::FLOOR && is_position_free(x, y))
		{
			return Vector2D{x, y};
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
	j["type"] = static_cast<int>(PickableType::TELEPORTER);
}

Pickable::PickableType Teleporter::get_type() const
{
	return PickableType::TELEPORTER;
}
