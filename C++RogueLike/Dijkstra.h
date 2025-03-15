#pragma once

#include <vector>
#include <queue>
#include <limits>

#include "Map/Map.h"
#include "Vector2D.h"

struct Node
{
    int vertex{};
    int weight{};
};

class Dijkstra
{
public:
    Dijkstra(int width, int height);
    void dijkstra_search(
        Map& graph,
        Vector2D start,
        Vector2D goal,
        std::vector<Vector2D>& cameFrom,
        std::vector<double>& costSoFar
    );
    std::vector<Vector2D> reconstruct_path(
        Vector2D start, Vector2D goal,
        std::vector<Vector2D> cameFrom
    );
private:
    int width, height;
    double infinity;
};

// Structure to represent a node in the priority queue
struct FrontierNode {
    Vector2D vertex{}; // The current node (position in grid)
    double weight{};   // The cost to reach this node

    // Comparator for priority queue (min-heap behavior)
    bool operator>(const FrontierNode& other) const {
        return weight > other.weight; // Lower cost = higher priority
    }
};
