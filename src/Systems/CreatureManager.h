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

	// Spawning
	void spawn_creatures(GameContext& ctx);

	// Queries
	Creature* get_closest_monster(
		std::span<const std::unique_ptr<Creature>> creatures,
		Vector2D fromPosition,
		int inRange) const noexcept;

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
