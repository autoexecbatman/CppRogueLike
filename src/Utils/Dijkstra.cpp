#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <vector>

#include "../Actor/Attacker.h"
#include "../Map/Map.h"
#include "Dijkstra.h"
#include "Vector2D.h"

Dijkstra::Dijkstra(int width, int height)
	: width(width), infinity(std::numeric_limits<double>::infinity())
{
	// Pre-allocate work vectors once; they'll be reused and cleared each search
	const size_t gridSize = static_cast<size_t>(width) * height;
	cameFrom_.resize(gridSize);
	costSoFar_.resize(gridSize);
}

// set AStar true for A* search, false for Dijkstra's search
// returns a vector of the path
std::vector<Vector2D> Dijkstra::a_star_search(
	Map& graph,
	Vector2D start,
	Vector2D goal,
	bool AStar,
	GameContext& ctx)
{
	// Reuse persistent work vectors; just reset them for this search
	// Clear and re-initialize the work vectors (fast: just fills existing capacity)
	// cameFrom[i] -> Stores which node led to this node
	// costSoFar[i] -> Stores the lowest cost found to reach this node
	std::fill(cameFrom_.begin(), cameFrom_.end(), Vector2D{ -1, -1 });
	std::fill(costSoFar_.begin(), costSoFar_.end(), infinity);

	// Priority queue for processing nodes, ordered by lowest cost (min-heap)
	std::priority_queue<FrontierNode, std::vector<FrontierNode>, std::greater<FrontierNode>> frontier;
	// Push the start node to the frontier
	frontier.emplace(start, 0);

	// Get the index of the starting position in the 1D vector representation of the grid
	const size_t startIndex = graph.get_index(start);
	cameFrom_[startIndex] = start; // The start node leads to itself
	costSoFar_[startIndex] = 0; // Cost to reach the start is 0

	// Process nodes until there are none left in the priority queue
	while (!frontier.empty())
	{
		// Get the node with the lowest cost from the priority queue
		Vector2D current = frontier.top().vertex;
		frontier.pop(); // Remove it from the queue

		// If we reached the goal, stop searching (optional optimization)
		if (current == goal)
		{
			break;
		}

		// Explore all valid neighbors of the current node (pass goal to allow reaching target)
		for (Vector2D next : graph.neighbors(current, ctx, goal))
		{
			const size_t currentIndex = graph.get_index(current); // Get index of the current node
			const size_t nextIndex = graph.get_index(next); // Get index of the next node

			// If this new path is better (lower cost) than the previously known cost
			double new_cost = costSoFar_[currentIndex] + graph.cost(current, next, ctx);

			if (costSoFar_[nextIndex] == infinity || new_cost < costSoFar_[nextIndex])
			{
				costSoFar_[nextIndex] = new_cost; // Update cost to reach this node
				if (AStar) // for A* search
				{
					double priority = new_cost + heuristic(next, goal);
					frontier.emplace(next, priority);
				}
				else // for Dijkstra's search
				{
					frontier.emplace(next, new_cost);
				}
				cameFrom_[nextIndex] = current;
			}
		}
	}
	return reconstruct_path(start, goal, cameFrom_);
}

std::vector<Vector2D> Dijkstra::reconstruct_path(
	Vector2D start, // The starting position
	Vector2D goal, // The goal position
	const std::vector<Vector2D>& cameFrom // Stores the previous node in the path
)
{
	std::vector<Vector2D> path; // Stores the reconstructed path
	Vector2D current = goal; // Start reconstruction from the goal

	// use labmda for index
	auto get_index = [this](Vector2D pos)
	{
		return pos.y * width + pos.x;
	};

	int goalIndex = get_index(goal);

	// Check if the goal is reachable by looking at its cameFrom value
	if (cameFrom.at(goalIndex) == Vector2D{ -1, -1 })
	{
		return path; // No path found, return empty vector
	}

	// Reconstruct the path by tracing back from goal to start
	while (current != start)
	{
		path.push_back(current); // Add the current node to the path
		int currentIndex = get_index(current); // Get the index of the current node
		current = cameFrom.at(currentIndex); // Move to the previous node
	}

	path.push_back(start); // Optional: Include the starting point
	std::reverse(path.begin(), path.end()); // Reverse the path to get it from start to goal
	return path;
}
