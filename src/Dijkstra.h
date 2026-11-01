// file: Dijkstra.h
// The owner's search, restored from 8b173641^: Dijkstra's algorithm, or A*
// when asked, over any grid whose two questions - which tiles neighbour this
// one, and what moving between two of them costs - come in as a SearchGraph.
// It knows no map and no rule. Pathfinding supplies both, so the flood behind
// every cost map and the click route run the same loop. The work vectors are
// sized once in the constructor and reused.
//
// Example, a corridor of five floor tiles, (1,1) to (5,1), on a 7 by 3 grid,
// every step costing 1 (the calls DijkstraTest makes):
//   Dijkstra search{ 7, 3 };   // the grid's width and height
//   // corridor: neighbours along row 1, a price of 1 per step
//   search.a_star_search(
//       corridor,
//       Vector2D{ 1, 1 },      // start
//       Vector2D{ 4, 1 },      // goal
//       SearchOrder::A_STAR);
//   // -> (1,1) (2,1) (3,1) (4,1), start and goal included
//   search.a_star_search(
//       corridor,
//       Vector2D{ 1, 1 },
//       std::nullopt,          // no goal: flood every tile it can reach
//       SearchOrder::DIJKSTRA);
//   // -> empty
//   search.cost_so_far(Vector2D{ 5, 1 }); // -> 4, read from that flood
//   search.cost_so_far(Vector2D{ 6, 1 }); // -> nullopt, never reached
#pragma once

#include <functional>
#include <limits>
#include <optional>
#include <vector>

#include "Vector2D.h"

namespace CppRL
{

	// The two questions the search asks of a grid. neighbours lists the tiles
	// a step may reach from one tile; cost prices the step between two of
	// them.
	struct SearchGraph
	{
		// The tiles one step from current may reach.
		std::function<std::vector<Vector2D>(Vector2D current)> neighbours{};
		// The price of the step from current to next.
		std::function<double(Vector2D current, Vector2D next)> cost{};
	};

	// Which order the frontier is drawn in: A* adds the heuristic to the cost
	// so far and needs a goal; Dijkstra orders by the cost so far alone.
	enum class SearchOrder
	{
		A_STAR,
		DIJKSTRA
	};

	// The search, with the work vectors it reuses from one call to the next.
	class Dijkstra
	{
	private:
		int width{ 0 };
		double infinity{ std::numeric_limits<double>::infinity() };
		// Persistent work vectors, reused across calls.
		std::vector<std::optional<Vector2D>> cameFrom{};
		std::vector<double> costSoFar{};

		// Where a tile sits in the work vectors, stored row after row.
		//
		// Example, on a 7 by 3 grid:
		//   get_index(Vector2D{ 4, 1 }); // -> 11
		[[nodiscard]] size_t get_index(Vector2D position) const;

	public:
		// Sizes the work vectors for a grid of that many columns and rows.
		//
		// Example:
		//   Dijkstra search{ 7, 3 }; // 21 tiles, none reached yet
		Dijkstra(int gridWidth, int gridHeight);

		// Searches from start and returns the cheapest path to the goal, start
		// and goal included, or an empty one when the goal cannot be reached.
		// With no goal the search runs until the frontier is empty, prices
		// every tile it can reach for cost_so_far, and returns an empty path.
		// A* requires a goal, because the heuristic measures to it.
		//
		// Example, the corridor in the file block:
		//   search.a_star_search(
		//       corridor,
		//       Vector2D{ 1, 1 },
		//       Vector2D{ 4, 1 },
		//       SearchOrder::A_STAR);
		//   // -> (1,1) (2,1) (3,1) (4,1)
		//   search.a_star_search(
		//       corridor,
		//       Vector2D{ 1, 1 },
		//       Vector2D{ 6, 1 },     // past the corridor's end
		//       SearchOrder::A_STAR);
		//   // -> empty
		[[nodiscard]] std::vector<Vector2D> a_star_search(
			const SearchGraph& graph,
			Vector2D start,
			std::optional<Vector2D> goal,
			SearchOrder order);

		// The path the last search found from start to goal, start first, or
		// an empty one when the goal was never reached.
		//
		// Example, after the corridor search to (4,1):
		//   search.reconstruct_path(Vector2D{ 1, 1 }, Vector2D{ 4, 1 });
		//   // -> (1,1) (2,1) (3,1) (4,1)
		//   search.reconstruct_path(Vector2D{ 1, 1 }, Vector2D{ 6, 1 });
		//   // -> empty
		[[nodiscard]] std::vector<Vector2D> reconstruct_path(
			Vector2D start,
			Vector2D goal) const;

		// What the last search paid to reach a tile, or nullopt when it never
		// reached it.
		//
		// Example, after the corridor flood from (1,1):
		//   search.cost_so_far(Vector2D{ 5, 1 }); // -> 4
		//   search.cost_so_far(Vector2D{ 1, 2 }); // -> nullopt, never reached
		[[nodiscard]] std::optional<double> cost_so_far(
			Vector2D position) const;

		// Admissible for eight-way movement, where a diagonal costs at least
		// what a cardinal costs, so it never overestimates the steps
		// remaining.
		//
		// Example:
		//   search.heuristic(Vector2D{ 2, 2 }, Vector2D{ 8, 5 }); // -> 6
		[[nodiscard]] double heuristic(Vector2D position, Vector2D goal) const
		{
			return static_cast<double>(position.chebyshev_distance_to(goal));
		}
	};

	// A node in the priority queue: a tile and what it is ordered by.
	struct FrontierNode
	{
		// The current node (position in grid).
		Vector2D vertex{};
		// The cost to reach this node, plus the heuristic under A*.
		double weight{ 0.0 };

		// Comparator for the priority queue (min-heap behaviour): lower cost
		// is higher priority.
		[[nodiscard]] bool operator>(const FrontierNode& other) const noexcept
		{
			return weight > other.weight;
		}
	};

} // namespace CppRL
