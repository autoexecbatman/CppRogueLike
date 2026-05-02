#include <cassert>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Random/RandomDice.h"
#include "../Utils/Vector2D.h"
#include "CreatureManager.h"
#include "SpawnUtils.h"

void CreatureManager::update_creatures(std::span<std::unique_ptr<Creature>> creatures, GameContext& ctx)
{
	for (const auto& creature : creatures)
	{
		assert(creature);
		creature->update(ctx);
	}
}

void CreatureManager::cleanup_dead_creatures(std::vector<std::unique_ptr<Creature>>& creatures)
{
	// Remove dead creatures from the game
	// This is called at safe points to avoid dangling references during combat
	std::erase_if(creatures, [](const auto& creature)
		{ return creature && creature->is_dead(); });
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
		assert(actor);
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
		assert(actor);
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
	if (ctx.rooms->empty())
	{
		throw std::runtime_error("rooms vector is empty!");
	}

	// Pick a random room; retry with a different room if it is full.
	while (true)
	{
		const size_t index = static_cast<size_t>(
			ctx.dice->roll(0, static_cast<int>(ctx.rooms->size()) - 1));
		auto pos = SpawnUtils::find_random_room_position(ctx.rooms->at(index), ctx);
		if (pos)
		{
			return *pos;
		}
	}
}
