#pragma once

#include <vector>
#include <queue>
#include <limits>

#include "Map/Map.h"
#include "Vector2D.h"

class Dijkstra
{
public:
    Dijkstra(int width, int height);
    std::vector<Vector2D> a_star_search(
        Map& graph,
        Vector2D start,
        Vector2D goal,
        bool AStar
    );
    std::vector<Vector2D> reconstruct_path(
        Vector2D start, Vector2D goal,
        std::vector<Vector2D> cameFrom
    );
	// Heuristic function for A* search
    double heuristic(Vector2D a, Vector2D b)
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }
private:
    int width, height;
    double infinity;
};

// Structure to represent a node in the priority queue
struct FrontierNode
{
    Vector2D vertex{}; // The current node (position in grid)
    double weight{};   // The cost to reach this node

    // Comparator for priority queue (min-heap behavior)
    bool operator>(const FrontierNode& other) const
    {
        return weight > other.weight; // Lower cost = higher priority
    }
};
