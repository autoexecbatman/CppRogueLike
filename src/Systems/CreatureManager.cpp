#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Map/DungeonRoom.h"
#include "../Map/Map.h"
#include "../Random/RandomDice.h"
#include "../Utils/Vector2D.h"
#include "CreatureManager.h"

void CreatureManager::update_creatures(std::span<std::unique_ptr<Creature>> creatures, GameContext& ctx)
{
	for (const auto& creature : creatures)
	{
		if (creature)
		{
			creature->update(ctx);
		}
	}
}

void CreatureManager::cleanup_dead_creatures(std::vector<std::unique_ptr<Creature>>& creatures)
{
	// Remove dead creatures from the game
	// This is called at safe points to avoid dangling references during combat
	std::erase_if(creatures, [](const auto& creature)
		{ return creature && creature->destructible && creature->is_dead(); });
}

void CreatureManager::spawn_creatures(GameContext& ctx)
{
	// INCREASED SPAWN RATE - add a new monster every spawn_rate turns (default 2)
	if (ctx.gameState->get_time() % spawnRate == 0)
	{
		if (can_spawn_creature(*ctx.creatures, maxCreatures))
		{
			Vector2D spawnPos = find_spawn_position(ctx);
			ctx.map->add_monster(spawnPos, ctx);
		}
	}
}

Creature* CreatureManager::get_closest_monster(
	std::span<const std::unique_ptr<Creature>> creatures,
	Vector2D fromPosition,
	int inRange) const noexcept
{
	Creature* closestMonster = nullptr;
	int bestDistance = std::numeric_limits<int>::max();

	for (const auto& actor : creatures)
	{
		if (!actor->is_dead())
		{
			const int distance = actor->get_tile_distance(fromPosition);
			if (distance < bestDistance && (distance <= inRange || inRange == 0))
			{
				bestDistance = distance;
				closestMonster = actor.get();
			}
		}
	}

	return closestMonster;
}

Creature* CreatureManager::get_actor_at_position(
	std::span<const std::unique_ptr<Creature>> creatures,
	Vector2D pos) const noexcept
{
	for (const auto& actor : creatures)
	{
		if (actor->position == pos)
		{
			return actor.get();
		}
	}
	return nullptr;
}

bool CreatureManager::can_spawn_creature(
	std::span<const std::unique_ptr<Creature>> creatures,
	int max_creatures) const noexcept
{
	return creatures.size() < static_cast<size_t>(max_creatures);
}

Vector2D CreatureManager::find_spawn_position(GameContext& ctx)
{
	const auto& dice = ctx.dice;
	if (ctx.rooms->empty())
	{
		throw std::runtime_error("rooms vector is empty!");
	}

	const size_t index = static_cast<size_t>(dice->roll(0, static_cast<int>(ctx.rooms->size()) - 1));
	const DungeonRoom& room = ctx.rooms->at(index);

	Vector2D pos{ dice->roll(room.col, room.col_end()), dice->roll(room.row, room.row_end()) };
	while (!ctx.map->can_walk(pos, ctx) || ctx.map->get_actor(pos, ctx) != nullptr)
	{
		pos.x = dice->roll(room.col, room.col_end());
		pos.y = dice->roll(room.row, room.row_end());
	}

	return pos;
}
