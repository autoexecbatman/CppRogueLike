#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <limits>
#include <numbers>
#include <optional>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "Creature.h"
#include "EquipmentSlot.h"
#include "GameContext.h"
#include "Map.h"
#include "Pathfinding.h"
#include "SpellTile.h"
#include "TileConfig.h"
#include "TileDefinition.h"
#include "TileFeature.h"
#include "Trap.h"

namespace Pathfinding
{

namespace
{

// Every move is to one of these. A diagonal is legal by design and costs what a
// cardinal step does.
constexpr std::array<Vector2D, 8> NEIGHBOURS{ DIR_N, DIR_S, DIR_W, DIR_E, DIR_NW, DIR_NE, DIR_SW, DIR_SE };

constexpr int STEP_COST = 1;
// A shut door takes one turn to open and one to pass.
constexpr int SHUT_DOOR_COST = 2;
constexpr int UNREACHED = std::numeric_limits<int>::max();

// A frontier entry for the Dijkstra floods and the A* route search, cheapest first.
struct Frontier
{
	double priority{ 0.0 };
	Vector2D position{};

	bool operator>(const Frontier& other) const noexcept
	{
		return priority > other.priority;
	}
};

using FrontierQueue = std::priority_queue<Frontier, std::vector<Frontier>, std::greater<Frontier>>;

// Where a tile sits in a grid stored row after row.
//
// Example:
//   index_of(Vector2D{ 3, 2 }, 20); // -> 43
size_t index_of(Vector2D position, int width)
{
	return static_cast<size_t>(position.y) * static_cast<size_t>(width) + static_cast<size_t>(position.x);
}

// Lets every trap and web on the tile act on the creature entering it, until one stops
// it. A feature that destroyed itself this turn is inert until the sweep.
//
// Example, a goblin stepping onto a hidden dart trap and then onto bare floor:
//   enter_features(goblin, trapTile, ctx);  // -> EntryResult::BLOCKED, the dart springs
//   enter_features(goblin, floorTile, ctx); // -> EntryResult::UNAFFECTED
EntryResult enter_features(Creature& creature, Vector2D destination, GameContext& ctx)
{
	assert(ctx.traps && "step: a creature moved on a level with no trap list");
	assert(ctx.spellTiles && "step: a creature moved on a level with no spell tile list");

	bool stopped = false;
	const auto enter_all = [&](auto& features)
	{
		for (const auto& feature : features)
		{
			assert(feature && "step: a floor container holds a null entry");

			if (stopped || feature->is_destroyed() || feature->position != destination)
			{
				continue;
			}

			if (feature->on_creature_enter(creature, ctx) == EntryResult::BLOCKED)
			{
				stopped = true;
			}
		}
	};
	enter_all(*ctx.traps);
	enter_all(*ctx.spellTiles);

	return stopped ? EntryResult::BLOCKED : EntryResult::UNAFFECTED;
}

} // namespace

Mobility::Mobility(std::set<TileType> enterableTypes)
	: enterable(std::move(enterableTypes))
{
}

bool Mobility::contains(TileType tileType) const noexcept
{
	return enterable.contains(tileType);
}

Mobility mobility_of(const Creature& creature, const TileConfig& tileConfig)
{
	std::set<TileType> enterable;

	// What the ground demands is data: tile_config.json says what blocks and what
	// carries a creature through it anyway.
	for (const auto& [tileType, definition] : tileConfig.get_tile_definitions())
	{
		const bool isBypassed = definition.hasBypassState && creature.has_bypass(definition.bypassState);
		if (!definition.blocksMovement || isBypassed)
		{
			enterable.insert(tileType);
		}
	}

	// A door is opened by hand, and the body plan is what says a creature has one.
	if (creature.has_slot(EquipmentSlot::RIGHT_HAND) || creature.has_slot(EquipmentSlot::LEFT_HAND))
	{
		enterable.insert(TileType::CLOSED_DOOR);
	}

	return Mobility{ std::move(enterable) };
}

std::optional<int> step_cost(Vector2D position, const Mobility& mobility, const GameContext& ctx)
{
	assert(ctx.map && "step_cost: a tile was priced with no map");
	const Map& map = *ctx.map;

	if (!map.is_in_bounds(position))
	{
		return std::nullopt;
	}

	const TileType tileType = map.get_tile_type(position);
	if (!mobility.contains(tileType))
	{
		return std::nullopt;
	}

	// A lock stops hands; the key, the pick and forcing the door belong to the player's own turn.
	if (tileType == TileType::CLOSED_DOOR && map.is_door_locked(position))
	{
		return std::nullopt;
	}

	if (map.find_decoration_at(position, ctx) != nullptr)
	{
		return std::nullopt;
	}

	return tileType == TileType::CLOSED_DOOR ? SHUT_DOOR_COST : STEP_COST;
}

bool can_enter(const Creature& creature, Vector2D position, const GameContext& ctx)
{
	assert(ctx.map && "Pathfinding::can_enter: called without a map");
	assert(ctx.tileConfig && "can_enter: asked with no tile configuration loaded");

	if (!step_cost(position, mobility_of(creature, *ctx.tileConfig), ctx))
	{
		return false;
	}

	return ctx.map->get_tile_type(position) != TileType::CLOSED_DOOR && ctx.map->get_actor(position, ctx) == nullptr;
}

StepResult step(Creature& creature, Vector2D destination, GameContext& ctx)
{
	assert(ctx.map && "Pathfinding::step: called without a map");
	assert(creature.position.chebyshev_distance_to(destination) == 1 && "step: the destination is not a neighbouring tile");
	assert(ctx.tileConfig && "step: a creature moved with no tile configuration loaded");

	if (!step_cost(destination, mobility_of(creature, *ctx.tileConfig), ctx) || ctx.map->get_actor(destination, ctx) != nullptr)
	{
		return StepResult::REFUSED;
	}

	// Priced means unlocked and the creature has hands, so the door opens.
	if (ctx.map->get_tile_type(destination) == TileType::CLOSED_DOOR)
	{
		[[maybe_unused]] const bool opened = ctx.map->open_door(destination, ctx);
		assert(opened && "step: a priced shut door refused to open");
		return StepResult::OPENED_DOOR;
	}

	if (enter_features(creature, destination, ctx) == EntryResult::BLOCKED)
	{
		return StepResult::STOPPED;
	}

	creature.position = destination;
	return StepResult::MOVED;
}

const Pathfinder::CostMap& Pathfinder::cost_map(Vector2D goal, const Mobility& mobility, const GameContext& ctx)
{
	assert(ctx.map && "cost_to: a cost map was asked for with no map");
	assert(ctx.gameState && "cost_to: a cost map was asked for with no clock to date it by");
	const Map& map = *ctx.map;
	const int clock = ctx.gameState->get_time();
	const long levelSeed = map.get_seed();

	// One map per mobility. It stays good while nothing it was flooded from has moved.
	const auto is_same_mobility = [&mobility](const CostMap& costMap)
	{
		return costMap.mobility == mobility;
	};
	auto found = std::ranges::find_if(costMaps, is_same_mobility);
	if (found != costMaps.end() && found->goal == goal && found->clock == clock && found->levelSeed == levelSeed && found->width == map.get_width())
	{
		return *found;
	}
	if (found == costMaps.end())
	{
		costMaps.push_back(CostMap{ .mobility = mobility });
		found = std::prev(costMaps.end());
	}

	CostMap& costMap = *found;
	costMap.goal = goal;
	costMap.clock = clock;
	costMap.levelSeed = levelSeed;
	costMap.width = map.get_width();
	costMap.costs.assign(static_cast<size_t>(map.get_width()) * static_cast<size_t>(map.get_height()), UNREACHED);

	if (!map.is_in_bounds(goal))
	{
		return costMap;
	}

	// Dijkstra outward from the goal until the frontier is empty; a tile costs what entering it does.
	FrontierQueue frontier;
	costMap.costs[index_of(goal, costMap.width)] = 0;
	frontier.push(Frontier{ 0.0, goal });

	while (!frontier.empty())
	{
		const Frontier top = frontier.top();
		frontier.pop();

		const int reached = costMap.costs[index_of(top.position, costMap.width)];
		if (static_cast<double>(reached) < top.priority)
		{
			continue;
		}

		for (const Vector2D& delta : NEIGHBOURS)
		{
			const Vector2D next = top.position + delta;
			const std::optional<int> price = step_cost(next, mobility, ctx);
			if (!price)
			{
				continue;
			}

			const size_t nextIndex = index_of(next, costMap.width);
			const int nextCost = reached + *price;
			if (nextCost < costMap.costs[nextIndex])
			{
				costMap.costs[nextIndex] = nextCost;
				frontier.push(Frontier{ static_cast<double>(nextCost), next });
			}
		}
	}

	return costMap;
}

std::optional<int> Pathfinder::cost_to(Vector2D goal, Vector2D from, const Mobility& mobility, const GameContext& ctx)
{
	assert(ctx.map && "Pathfinder::cost_to: called without a map");

	const CostMap& costMap = cost_map(goal, mobility, ctx);

	if (!ctx.map->is_in_bounds(from))
	{
		return std::nullopt;
	}

	const int cost = costMap.costs[index_of(from, costMap.width)];
	if (cost == UNREACHED)
	{
		return std::nullopt;
	}
	return cost;
}

std::vector<Vector2D> Pathfinder::find_route(Vector2D start, Vector2D goal, const Mobility& mobility, const GameContext& ctx) const
{
	assert(ctx.map && "find_route: a route was asked for with no map");
	const Map& map = *ctx.map;
	assert(map.is_in_bounds(start) && "find_route: the route starts off the map");

	if (!map.is_in_bounds(goal))
	{
		return {};
	}

	const int width = map.get_width();
	const size_t cellCount = static_cast<size_t>(width) * static_cast<size_t>(map.get_height());
	constexpr double UNSEEN = std::numeric_limits<double>::infinity();
	std::vector<double> costSoFar(cellCount, UNSEEN);
	std::vector<std::optional<Vector2D>> cameFrom(cellCount, std::nullopt);

	// The goal is let in where the rule alone refuses it: a creature may stand there, a shut door
	// is somewhere to walk up to, and a decoration is bumped from the last step.
	const auto price_of = [&](Vector2D position) -> std::optional<double>
	{
		if (position == goal)
		{
			if (map.get_tile_type(goal) == TileType::CLOSED_DOOR)
			{
				return static_cast<double>(SHUT_DOOR_COST);
			}
			if (map.find_decoration_at(goal, ctx) != nullptr && mobility.contains(map.get_tile_type(goal)))
			{
				return static_cast<double>(STEP_COST);
			}
			const std::optional<int> goalPrice = step_cost(goal, mobility, ctx);
			return goalPrice ? std::optional<double>{ static_cast<double>(*goalPrice) } : std::nullopt;
		}
		if (map.get_actor(position, ctx) != nullptr)
		{
			return std::nullopt;
		}
		const std::optional<int> price = step_cost(position, mobility, ctx);
		return price ? std::optional<double>{ static_cast<double>(*price) } : std::nullopt;
	};

	// A*: the frontier is ordered by cost so far plus the heuristic, and stops at the goal.
	FrontierQueue frontier;
	costSoFar[index_of(start, width)] = 0.0;
	cameFrom[index_of(start, width)] = start;
	frontier.push(Frontier{ 0.0, start });

	while (!frontier.empty())
	{
		const Vector2D current = frontier.top().position;
		frontier.pop();

		if (current == goal)
		{
			break;
		}

		for (const Vector2D& delta : NEIGHBOURS)
		{
			const Vector2D next = current + delta;
			if (!map.is_in_bounds(next))
			{
				continue;
			}
			const std::optional<double> price = price_of(next);
			if (!price)
			{
				continue;
			}

			// A diagonal is weighted by its length so the route drawn on screen runs
			// straight; the step count is the same either way.
			const bool isDiagonal = delta.x != 0 && delta.y != 0;
			const double stepPrice = isDiagonal ? *price * std::numbers::sqrt2 : *price;
			const double newCost = costSoFar[index_of(current, width)] + stepPrice;
			const size_t nextIndex = index_of(next, width);
			if (newCost < costSoFar[nextIndex])
			{
				costSoFar[nextIndex] = newCost;
				cameFrom[nextIndex] = current;
				const double heuristic = static_cast<double>(next.chebyshev_distance_to(goal));
				frontier.push(Frontier{ newCost + heuristic, next });
			}
		}
	}

	if (!cameFrom[index_of(goal, width)])
	{
		return {};
	}

	std::vector<Vector2D> route{ goal };
	while (route.back() != start)
	{
		route.push_back(*cameFrom[index_of(route.back(), width)]);
	}
	std::ranges::reverse(route);
	return route;
}

} // namespace Pathfinding
