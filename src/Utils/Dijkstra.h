#pragma once

#include <cmath>
#include <vector>

#include "../Map/Map.h"
#include "Vector2D.h"

struct GameContext;

class Dijkstra
{
public:
	Dijkstra(int width, int height);
	std::vector<Vector2D> a_star_search(
		Map& graph,
		Vector2D start,
		Vector2D goal,
		bool AStar,
		GameContext& ctx);
	std::vector<Vector2D> reconstruct_path(
		Vector2D start, Vector2D goal, const std::vector<Vector2D>& cameFrom);
	// Heuristic function for A* search
	double heuristic(Vector2D a, Vector2D b)
	{
		// Chebyshev distance (max of absolute differences)
		// Admissible for 8-directional movement where diagonals cost same as cardinal
		int dx = std::abs(a.x - b.x);
		int dy = std::abs(a.y - b.y);
		return static_cast<double>(std::max(dx, dy));
	}

private:
	int width;
	double infinity;
	std::vector<Vector2D> cameFrom_;      // Persistent work vector (reused across calls)
	std::vector<double> costSoFar_;       // Persistent work vector (reused across calls)
};

// Structure to represent a node in the priority queue
struct FrontierNode
{
	Vector2D vertex{}; // The current node (position in grid)
	double weight{}; // The cost to reach this node

	// Comparator for priority queue (min-heap behavior)
	bool operator>(const FrontierNode& other) const
	{
		return weight > other.weight; // Lower cost = higher priority
	}
};
