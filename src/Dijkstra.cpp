// file: Dijkstra.cpp
// The owner's search loop and path reconstruction, restored from
// 8b173641^. Usage is in Dijkstra.h.

#include <algorithm>
#include <cassert>
#include <functional>
#include <optional>
#include <queue>
#include <vector>

#include "Dijkstra.h"
#include "Vector2D.h"

namespace CppRL
{

	Dijkstra::Dijkstra(int gridWidth, int gridHeight) :
		width(gridWidth)
	{
		assert(gridWidth > 0 && gridHeight > 0 && "Dijkstra: an empty grid");
		// Pre-allocate work vectors once; they are reused and cleared each
		// search.
		const size_t gridSize =
			static_cast<size_t>(gridWidth) * static_cast<size_t>(gridHeight);
		cameFrom.resize(gridSize);
		costSoFar.resize(gridSize);
	}

	size_t Dijkstra::get_index(Vector2D position) const
	{
		const size_t index =
			static_cast<size_t>(position.y) * static_cast<size_t>(width) +
			static_cast<size_t>(position.x);
		assert(index < costSoFar.size() && "Dijkstra: a tile off the grid");
		return index;
	}

	std::vector<Vector2D> Dijkstra::a_star_search(
		const SearchGraph& graph,
		Vector2D start,
		std::optional<Vector2D> goal,
		SearchOrder order)
	{
		assert(
			(order == SearchOrder::DIJKSTRA || goal) &&
			"a_star_search: A* needs a goal to measure the heuristic to");

		// Reset the work vectors for this search: cameFrom holds which node led
		// to each node, costSoFar the lowest cost found to reach it.
		std::ranges::fill(cameFrom, std::nullopt);
		std::ranges::fill(costSoFar, infinity);

		// Priority queue for processing nodes, ordered by lowest cost
		// (min-heap).
		std::priority_queue<
			FrontierNode,
			std::vector<FrontierNode>,
			std::greater<FrontierNode>>
			frontier;
		frontier.emplace(start, 0.0);

		// The start node leads to itself, at no cost.
		const size_t startIndex = get_index(start);
		cameFrom[startIndex] = start;
		costSoFar[startIndex] = 0;

		// Process nodes until there are none left in the priority queue.
		while (!frontier.empty())
		{
			// The node with the lowest cost, removed from the queue.
			const Vector2D current = frontier.top().vertex;
			frontier.pop();

			// Reaching the goal ends the search; with no goal it runs dry.
			if (goal && current == *goal)
			{
				break;
			}

			for (const Vector2D next : graph.neighbours(current))
			{
				const size_t currentIndex = get_index(current);
				const size_t nextIndex = get_index(next);

				// Kept only when this path is cheaper than any found before.
				const double newCost =
					costSoFar[currentIndex] + graph.cost(current, next);
				if (costSoFar[nextIndex] == infinity ||
					newCost < costSoFar[nextIndex])
				{
					costSoFar[nextIndex] = newCost;
					if (order == SearchOrder::A_STAR)
					{
						const double priority =
							newCost + heuristic(next, *goal);
						frontier.emplace(next, priority);
					}
					else
					{
						frontier.emplace(next, newCost);
					}
					cameFrom[nextIndex] = current;
				}
			}
		}

		if (!goal)
		{
			return {};
		}
		return reconstruct_path(start, *goal);
	}

	std::vector<Vector2D> Dijkstra::reconstruct_path(
		Vector2D start,
		Vector2D goal) const
	{
		std::vector<Vector2D> path;
		// Reconstruction starts from the goal.
		Vector2D current = goal;

		// A goal nothing led to was never reached.
		const size_t goalIndex = get_index(goal);
		if (!cameFrom.at(goalIndex))
		{
			return path;
		}

		// Trace back from the goal to the start.
		while (current != start)
		{
			path.push_back(current);
			const size_t currentIndex = get_index(current);
			current = *cameFrom.at(currentIndex);
		}

		// The start is part of the path, which is then put start first.
		path.push_back(start);
		std::ranges::reverse(path);
		return path;
	}

	std::optional<double> Dijkstra::cost_so_far(Vector2D position) const
	{
		const double cost = costSoFar.at(get_index(position));
		if (cost == infinity)
		{
			return std::nullopt;
		}
		return cost;
	}

} // namespace CppRL
