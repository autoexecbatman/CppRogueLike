// file: Pathfinding.h
// The one rule for where a creature may go, and everything that plans or makes a move by
// it. A creature's mobility is the set of tile types it may enter, worked out from the
// creature each time it is asked: open ground, water when it swims, a shut door when its
// body has hands. step_cost prices a tile for a mobility; the cost maps monsters descend
// and the player's click-to-walk route both read it, and step is the one way any creature
// enters a neighbouring tile. The capability idea is Harabor and Botea's (CIG 2008).
//
// Example, a goblin (a humanoid body, no swimming) beside floor, a shut door and a pool:
//   const Pathfinding::Mobility walker = Pathfinding::mobility_of(goblin, tileConfig);
//   walker.contains(TileType::CLOSED_DOOR);         // -> true, it has hands
//   walker.contains(TileType::WATER);               // -> false
//   Pathfinding::step_cost(floorTile, walker, ctx); // -> 1
//   Pathfinding::step_cost(shutDoor, walker, ctx);  // -> 2, a turn to open and one to pass
//   Pathfinding::step_cost(pool, walker, ctx);      // -> nullopt
//   Pathfinding::can_enter(goblin, shutDoor, ctx);  // -> false, a door is opened first
//   Pathfinding::step(goblin, shutDoor, ctx);       // -> StepResult::OPENED_DOOR
//   Pathfinding::step(goblin, shutDoor, ctx);       // -> StepResult::MOVED
//
// Two searches read that price. A cost map is Dijkstra's algorithm run outward from the
// goal over the whole level - a Dijkstra map, in roguelike usage - so every monster
// descends one map. A route is A* from one tile to one goal, for the player's click.
//
// The cost maps and routes live on the Pathfinder at ctx.pathfinder. Along a corridor
// with the player at (2,5) and a shut door at (9,5):
//   ctx.pathfinder->cost_to(Vector2D{ 2, 5 }, Vector2D{ 8, 5 }, walker, ctx);  // -> 6, six paces
//   ctx.pathfinder->cost_to(Vector2D{ 2, 5 }, Vector2D{ 12, 5 }, walker, ctx); // -> 11, the door is two
//   ctx.pathfinder->cost_to(Vector2D{ 2, 5 }, Vector2D{ 12, 5 }, wolf, ctx);   // -> nullopt, no hands
//   ctx.pathfinder->find_route(Vector2D{ 2, 5 }, Vector2D{ 16, 5 }, walker, ctx); // -> (2,5)..(16,5)
//   ctx.pathfinder->find_route(Vector2D{ 2, 5 }, Vector2D{ 16, 5 }, wolf, ctx);   // -> empty
#pragma once

#include <optional>
#include <set>
#include <vector>

#include "TileType.h"
#include "Vector2D.h"

class Creature;
class TileConfig;
struct GameContext;

namespace Pathfinding
{

// The tile types a creature may enter. Compared by value, so two creatures that move
// alike share one cost map.
class Mobility
{
private:
	std::set<TileType> enterable{};

public:
	explicit Mobility(std::set<TileType> enterableTypes);

	[[nodiscard]] bool contains(TileType tileType) const noexcept;
	[[nodiscard]] bool operator==(const Mobility& other) const noexcept = default;
};

// What one step did. A creature that opened a door or was stopped by what lies on the
// tile stays where it was.
enum class StepResult
{
	MOVED,
	OPENED_DOOR,
	STOPPED,
	REFUSED
};

// The creature's mobility: every tile type tile_config.json does not mark as blocking,
// each blocking type whose bypass state the creature has from itself or what it wears,
// and the shut door when its body plan has a hand slot.
[[nodiscard]] Mobility mobility_of(const Creature& creature, const TileConfig& tileConfig);

// What entering that tile costs a creature of that mobility: 1, or 2 for a shut door.
// nullopt for a tile off the map, a type outside the mobility, a locked door or a tile a
// decoration stands on. Creatures, traps and webs are no part of the price.
[[nodiscard]] std::optional<int> step_cost(Vector2D position, const Mobility& mobility, const GameContext& ctx);

// Whether the creature may stand on that tile now: priced for its mobility, not a shut
// door, and no creature on it.
[[nodiscard]] bool can_enter(const Creature& creature, Vector2D position, const GameContext& ctx);

// The creature's step onto an adjacent tile. A shut door it may open is opened instead
// of entered. Otherwise every trap and web on the tile has its say, and the creature
// moves unless one of them stops it. Refused, with nothing changed, when the tile is not
// priced for the creature or a creature stands on it. Requires the destination to be one
// of the eight neighbours.
StepResult step(Creature& creature, Vector2D destination, GameContext& ctx);

// The Dijkstra maps monsters descend and the A* route the player's click walks.
class Pathfinder
{
private:
	struct CostMap
	{
		Mobility mobility;
		Vector2D goal{};
		int clock{ 0 };
		long levelSeed{ 0 };
		int width{ 0 };
		std::vector<int> costs{};
	};

	std::vector<CostMap> costMaps{};

	const CostMap& cost_map(Vector2D goal, const Mobility& mobility, const GameContext& ctx);

public:
	// The cost of the cheapest way from a tile to the goal for that mobility, or nullopt
	// when there is none, read from a Dijkstra map: Dijkstra's algorithm from the goal,
	// with no target, so it prices every tile. A map is flooded on first request and
	// reused while the goal, the level and the game clock are unchanged, so each monster
	// window floods once per mobility.
	[[nodiscard]] std::optional<int> cost_to(Vector2D goal, Vector2D from, const Mobility& mobility, const GameContext& ctx);

	// The cheapest route from start to goal, both included, for that mobility, found by A*:
	// a diagonal costs its price times sqrt 2, and the Chebyshev distance to the goal is
	// the heuristic, which never overestimates because no step costs less than 1. Tiles a
	// creature stands on are avoided, except the goal. The goal may be any shut door,
	// locked or not, because walking to a door is how its lock is reached, and any
	// decoration, because the last step bumps it. Empty when the goal cannot be reached.
	[[nodiscard]] std::vector<Vector2D> find_route(Vector2D start, Vector2D goal, const Mobility& mobility, const GameContext& ctx) const;
};

} // namespace Pathfinding
