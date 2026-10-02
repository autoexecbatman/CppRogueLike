#pragma once

#include <iterator>
#include <memory>
#include <span>
#include <vector>

// Forward declarations
class Creature;
class Map;
class RandomDice;
struct Vector2D;
struct DungeonRoom;
struct GameContext;

// - Handles all creature lifecycle and management
class CreatureManager
{
public:
	// Creature lifecycle
	void update_creatures(std::span<std::unique_ptr<Creature>> creatures, GameContext& ctx);
	void cleanup_dead_creatures(std::vector<std::unique_ptr<Creature>>& creatures);

	// Puts a creature on the level and on the clock, at the moment it arrives. The
	// one way a creature enters play: pushing onto ctx.creatures directly leaves it
	// standing at time zero, and the schedule would then owe it every action since
	// the game began.
	//
	// The save loader is the exception and builds its vector itself, because a
	// loaded creature already carries the place it had.
	//
	// Example:
	//   ctx.creatureManager->add_creature(MonsterCreator::create(pos, id, ctx), ctx);
	void add_creature(std::unique_ptr<Creature> creature, GameContext& ctx);

	// Spawning
	void spawn_creatures(GameContext& ctx);

	// Queries
	Creature* get_actor_at_position(
		std::span<const std::unique_ptr<Creature>> creatures,
		Vector2D pos) const noexcept;

private:
	int maxCreatures{ 10 };
	int spawnRate{ 2 };

	// Helper methods
	bool can_spawn_creature(
		std::span<const std::unique_ptr<Creature>> creatures,
		int max_creatures) const noexcept;

	Vector2D find_spawn_position(GameContext& ctx);
};
