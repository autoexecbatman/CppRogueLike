#pragma once

#include <cmath>
#include <vector>

#include "Map.h"
#include "Vector2D.h"

struct GameContext;

class Dijkstra
{
private:
	int width;
	double infinity;
	std::vector<Vector2D> cameFrom; // Persistent work vector (reused across calls)
	std::vector<double> costSoFar; // Persistent work vector (reused across calls)

public:
	Dijkstra(int width, int height);
	std::vector<Vector2D> a_star_search(
		Map& graph,
		Vector2D start,
		Vector2D goal,
		bool AStar,
		const GameContext& ctx);
	std::vector<Vector2D> reconstruct_path(
		Vector2D start, Vector2D goal, const std::vector<Vector2D>& cameFrom);
	// Admissible for eight-way movement, where a diagonal costs what a cardinal
	// costs, so it never overestimates the steps remaining.
	double heuristic(Vector2D a, Vector2D b)
	{
		return static_cast<double>(a.chebyshev_distance_to(b));
	}
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
