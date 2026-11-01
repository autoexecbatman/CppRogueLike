#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <limits>
#include <numbers>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "Creature.h"
#include "Dijkstra.h"
#include "EquipmentSlot.h"
#include "GameContext.h"
#include "Map.h"
#include "Pathfinding.h"
#include "SpellTile.h"
#include "TileConfig.h"
#include "TileFeature.h"
#include "TileType.h"
#include "Trap.h"
#include "Vector2D.h"

namespace CppRL
{

	namespace Pathfinding
	{

		namespace
		{

			// Every move is to one of these. A diagonal is legal by design and
			// costs what a cardinal step does.
			constexpr std::array<Vector2D, 8> NEIGHBOURS{
				DIR_N,
				DIR_S,
				DIR_W,
				DIR_E,
				DIR_NW,
				DIR_NE,
				DIR_SW,
				DIR_SE,
			};

			constexpr int STEP_COST = 1;
			// A shut door takes one turn to open and one to pass.
			constexpr int SHUT_DOOR_COST = 2;
			constexpr int UNREACHED = std::numeric_limits<int>::max();

			// Where a tile sits in a grid stored row after row.
			//
			// Example:
			//   index_of(Vector2D{ 3, 2 }, 20); // -> 43
			size_t index_of(Vector2D position, int width)
			{
				return static_cast<size_t>(position.y) *
					static_cast<size_t>(width) +
					static_cast<size_t>(position.x);
			}

			// Lets every trap and web on the tile act on the creature entering
			// it, until one stops it. A feature that destroyed itself this turn
			// is inert until the sweep.
			//
			// Example, a goblin stepping onto a hidden dart trap and then onto
			// bare
			// floor: enter_features(goblin, trapTile, ctx);  // ->
			// EntryResult::BLOCKED, the dart springs enter_features(goblin,
			// floorTile, ctx); // -> EntryResult::UNAFFECTED
			EntryResult enter_features(
				Creature& creature,
				Vector2D destination,
				GameContext& ctx)
			{
				assert(
					ctx.traps &&
					"step: a creature moved on a level with no trap list");
				assert(
					ctx.spellTiles &&
					"step: a creature moved on a level with no spell tile list");

				bool stopped = false;
				const auto enter_all = [&](auto& features)
				{
					for (const auto& feature : features)
					{
						assert(
							feature &&
							"step: a floor container holds a null entry");

						if (stopped || feature->is_destroyed() ||
							feature->position != destination)
						{
							continue;
						}

						if (feature->on_creature_enter(creature, ctx) ==
							EntryResult::BLOCKED)
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

		Mobility::Mobility(std::set<TileType> enterableTypes) :
			enterable(std::move(enterableTypes))
		{
		}

		bool Mobility::contains(TileType tileType) const noexcept
		{
			return enterable.contains(tileType);
		}

		Mobility mobility_of(
			const Creature& creature,
			const TileConfig& tileConfig)
		{
			std::set<TileType> enterable;

			// What the ground demands is data: tile_config.json says what
			// blocks and what carries a creature through it anyway.
			for (
				const auto& [tileType, definition] :
				tileConfig.get_tile_definitions())
			{
				const bool isBypassed = definition.hasBypassState &&
					creature.has_bypass(definition.bypassState);
				if (!definition.blocksMovement || isBypassed)
				{
					enterable.insert(tileType);
				}
			}

			// A door is opened by hand, and the body plan is what says a
			// creature has one.
			if (creature.has_slot(EquipmentSlot::RIGHT_HAND) ||
				creature.has_slot(EquipmentSlot::LEFT_HAND))
			{
				enterable.insert(TileType::CLOSED_DOOR);
			}

			return Mobility{ std::move(enterable) };
		}

		std::optional<int> step_cost(
			Vector2D position,
			const Mobility& mobility,
			const GameContext& ctx)
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

			// A lock stops hands; the key, the pick and forcing the door belong
			// to the player's own turn.
			if (tileType == TileType::CLOSED_DOOR &&
				map.is_door_locked(position))
			{
				return std::nullopt;
			}

			if (map.find_decoration_at(position, ctx) != nullptr)
			{
				return std::nullopt;
			}

			const bool isShutDoor = tileType == TileType::CLOSED_DOOR;
			return isShutDoor ? SHUT_DOOR_COST : STEP_COST;
		}

		bool can_enter(
			const Creature& creature,
			Vector2D position,
			const GameContext& ctx)
		{
			assert(ctx.map && "Pathfinding::can_enter: called without a map");
			assert(
				ctx.tileConfig &&
				"can_enter: asked with no tile configuration loaded");

			if (!step_cost(
					position,
					mobility_of(creature, *ctx.tileConfig),
					ctx))
			{
				return false;
			}

			return ctx.map->get_tile_type(position) != TileType::CLOSED_DOOR &&
				ctx.map->get_actor(position, ctx) == nullptr;
		}

		StepResult step(
			Creature& creature,
			Vector2D destination,
			GameContext& ctx)
		{
			assert(ctx.map && "Pathfinding::step: called without a map");
			assert(
				creature.position.chebyshev_distance_to(destination) == 1 &&
				"step: the destination is not a neighbouring tile");
			assert(
				ctx.tileConfig &&
				"step: a creature moved with no tile configuration loaded");

			if (!step_cost(
					destination,
					mobility_of(creature, *ctx.tileConfig),
					ctx) ||
				ctx.map->get_actor(destination, ctx) != nullptr)
			{
				return StepResult::REFUSED;
			}

			// Priced means unlocked and the creature has hands, so the door
			// opens.
			if (ctx.map->get_tile_type(destination) == TileType::CLOSED_DOOR)
			{
				[[maybe_unused]] const bool opened =
					ctx.map->open_door(destination, ctx);
				assert(opened && "step: a priced shut door refused to open");
				return StepResult::OPENED_DOOR;
			}

			if (enter_features(creature, destination, ctx) ==
				EntryResult::BLOCKED)
			{
				return StepResult::STOPPED;
			}

			creature.position = destination;
			return StepResult::MOVED;
		}

		const Pathfinder::CostMap& Pathfinder::cost_map(
			Vector2D goal,
			const Mobility& mobility,
			const GameContext& ctx)
		{
			assert(ctx.map && "cost_to: a cost map was asked for with no map");
			assert(
				ctx.gameState &&
				"cost_to: a cost map was asked for with no clock to date it by");
			const Map& map = *ctx.map;
			const int clock = ctx.gameState->get_time();
			const long levelSeed = map.get_seed();

			// One map per mobility. It stays good while nothing it was flooded
			// from has moved.
			const auto is_same_mobility = [&mobility](const CostMap& costMap)
			{
				return costMap.mobility == mobility;
			};
			auto found = std::ranges::find_if(costMaps, is_same_mobility);
			if (found != costMaps.end() && found->goal == goal &&
				found->clock == clock && found->levelSeed == levelSeed &&
				found->width == map.get_width())
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
			costMap.costs.assign(
				static_cast<size_t>(map.get_width()) *
					static_cast<size_t>(map.get_height()),
				UNREACHED);

			if (!map.is_in_bounds(goal))
			{
				return costMap;
			}

			// Dijkstra outward from the goal with no target, so it prices every
			// tile; a tile costs what entering it does.
			const auto priced_neighbours = [&mobility, &ctx](Vector2D current)
			{
				std::vector<Vector2D> priced{};
				for (const Vector2D& delta : NEIGHBOURS)
				{
					const Vector2D next = current + delta;
					if (step_cost(next, mobility, ctx))
					{
						priced.push_back(next);
					}
				}
				return priced;
			};
			const auto entry_price =
				[&mobility,
					&ctx]([[maybe_unused]] Vector2D current, Vector2D next)
			{
				assert(
					current.chebyshev_distance_to(next) == 1 &&
					"cost_map: a step priced between tiles that do not touch");
				const std::optional<int> price = step_cost(next, mobility, ctx);
				assert(price && "cost_map: a neighbour listed with no price");
				return static_cast<double>(*price);
			};
			Dijkstra& dijkstra =
				search_sized_for(map.get_width(), map.get_height());
			[[maybe_unused]] const std::vector<Vector2D> noPath =
				dijkstra.a_star_search(
					SearchGraph{
						.neighbours = priced_neighbours,
						.cost = entry_price,
					},
					goal,
					std::nullopt,
					SearchOrder::DIJKSTRA);
			assert(
				noPath.empty() && "cost_map: a flood with no goal drew a path");

			// Every cost is a sum of whole step prices, so the double is exact.
			for (int row = 0; row < map.get_height(); ++row)
			{
				for (int col = 0; col < map.get_width(); ++col)
				{
					const Vector2D tile{ col, row };
					const std::optional<double> cost =
						dijkstra.cost_so_far(tile);
					if (cost)
					{
						costMap.costs[index_of(tile, costMap.width)] =
							static_cast<int>(*cost);
					}
				}
			}

			return costMap;
		}

		std::optional<int> Pathfinder::cost_to(
			Vector2D goal,
			Vector2D from,
			const Mobility& mobility,
			const GameContext& ctx)
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

		std::vector<Vector2D> Pathfinder::find_route(
			Vector2D start,
			Vector2D goal,
			const Mobility& mobility,
			const GameContext& ctx)
		{
			assert(ctx.map && "find_route: a route was asked for with no map");
			const Map& map = *ctx.map;
			assert(
				map.is_in_bounds(start) &&
				"find_route: the route starts off the map");

			if (!map.is_in_bounds(goal))
			{
				return {};
			}

			// The goal is let in where the rule alone refuses it: a creature
			// may stand there, a shut door is somewhere to walk up to, and a
			// decoration is bumped from the last step.
			const auto price_of =
				[&](Vector2D position) -> std::optional<double>
			{
				if (position == goal)
				{
					if (map.get_tile_type(goal) == TileType::CLOSED_DOOR)
					{
						return static_cast<double>(SHUT_DOOR_COST);
					}
					if (map.find_decoration_at(goal, ctx) != nullptr &&
						mobility.contains(map.get_tile_type(goal)))
					{
						return static_cast<double>(STEP_COST);
					}
					const std::optional<int> goalPrice =
						step_cost(goal, mobility, ctx);
					if (!goalPrice)
					{
						return std::nullopt;
					}
					return static_cast<double>(*goalPrice);
				}
				if (map.get_actor(position, ctx) != nullptr)
				{
					return std::nullopt;
				}
				const std::optional<int> price =
					step_cost(position, mobility, ctx);
				if (!price)
				{
					return std::nullopt;
				}
				return static_cast<double>(*price);
			};

			// A*: the frontier is ordered by cost so far plus the heuristic,
			// and stops at the goal.
			const auto open_neighbours = [&map, &price_of](Vector2D current)
			{
				std::vector<Vector2D> open{};
				for (const Vector2D& delta : NEIGHBOURS)
				{
					const Vector2D next = current + delta;
					if (map.is_in_bounds(next) && price_of(next))
					{
						open.push_back(next);
					}
				}
				return open;
			};
			const auto step_price = [&price_of](Vector2D current, Vector2D next)
			{
				const std::optional<double> price = price_of(next);
				assert(price && "find_route: a neighbour listed with no price");
				// A diagonal is weighted by its length so the route drawn on
				// screen runs straight; the step count is the same either way.
				const Vector2D delta = next - current;
				const bool isDiagonal = delta.x != 0 && delta.y != 0;
				return isDiagonal ? *price * std::numbers::sqrt2 : *price;
			};
			Dijkstra& dijkstra =
				search_sized_for(map.get_width(), map.get_height());
			return dijkstra.a_star_search(
				SearchGraph{
					.neighbours = open_neighbours,
					.cost = step_price,
				},
				start,
				goal,
				SearchOrder::A_STAR);
		}

		Dijkstra& Pathfinder::search_sized_for(int width, int height)
		{
			if (!search || searchWidth != width || searchHeight != height)
			{
				search.emplace(width, height);
				searchWidth = width;
				searchHeight = height;
			}
			return *search;
		}

	} // namespace Pathfinding

} // namespace CppRL
