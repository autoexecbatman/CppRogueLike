#include <cassert>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include "Creature.h"
#include "CreatureManager.h"
#include "GameContext.h"
#include "Map.h"
#include "RandomDice.h"
#include "SpawnUtils.h"
#include "TurnSchedule.h"
#include "Vector2D.h"
#include <optional>

void CreatureManager::add_creature(std::unique_ptr<Creature> creature, GameContext& ctx)
{
	assert(creature && "add_creature: nothing to add");
	assert(ctx.creatures && "add_creature: a creature arrived with no level to arrive on");
	assert(ctx.gameState && "add_creature: a creature arrived with no clock to stand on");

	// Arriving now means standing where the clock stands: due at this moment, which
	// is what every creature already on the level is.
	creature->set_next_action_time(ctx.gameState->get_time());
	ctx.creatures->push_back(std::move(creature));
}

void CreatureManager::update_creatures(std::span<std::unique_ptr<Creature>> creatures, GameContext& ctx)
{
	assert(ctx.gameState && "update_creatures: the creatures acted with no clock");

	// How far this window reaches. The clock is where the player now stands, so each
	// creature takes the actions it is due before the player comes round again: one
	// for an ordinary creature, several for a faster one, none for one that overshot.
	const int clockLimit = ctx.gameState->get_time();

	for (const auto& creature : creatures)
	{
		assert(creature && "update_creatures: the creature list holds a null entry");

		const int cost = creature->get_action_delay();
		const int due = creature->scheduled_actions_before(clockLimit);

		for (int action = 0; action < due; ++action)
		{
			creature->update(ctx);
			creature->set_next_action_time(creature->get_next_action_time() + cost);

			// A creature killed by what it just did takes no further action, and the
			// sweep after this loop removes it.
			if (creature->is_dead())
			{
				break;
			}
		}
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
	// One new monster every spawnRate rounds. Read off the clock as rounds rather
	// than as time units: the clock advances by a hundred or so per action, so a
	// remainder taken on it directly would be zero almost every turn.
	if (rounds_completed_at(ctx.gameState->get_time()) % spawnRate == 0)
	{
		if (can_spawn_creature(*ctx.creatures, maxCreatures))
		{
			// Nowhere to put one is a legal outcome: the round passes without a monster.
			const std::optional<Vector2D> spawnPos = find_spawn_position(ctx);
			if (spawnPos)
			{
				ctx.map->add_monster(*spawnPos, ctx);
			}
		}
	}
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

std::optional<Vector2D> CreatureManager::find_spawn_position(GameContext& ctx)
{
	// A level with no rooms was never built. That is an invariant rather than a state to
	// branch on, and the empty walk below answers nullopt if it is ever compiled out.
	assert(ctx.rooms && !ctx.rooms->empty() && "find_spawn_position: a level with no rooms");

	const size_t roomCount = ctx.rooms->size();
	if (roomCount == 0)
	{
		return std::nullopt;
	}

	// A random start and then once round, so every room is tried and none is tried twice.
	// Picking a fresh random room each time could not tell a full dungeon from bad luck,
	// and spun forever on the first.
	const size_t start = static_cast<size_t>(ctx.dice->roll(0, static_cast<int>(roomCount) - 1));
	for (size_t offset = 0; offset < roomCount; ++offset)
	{
		const DungeonRoom& room = ctx.rooms->at((start + offset) % roomCount);
		const std::optional<Vector2D> pos = SpawnUtils::find_random_room_position(room, ctx);
		if (pos)
		{
			return pos;
		}
	}

	return std::nullopt;
}
