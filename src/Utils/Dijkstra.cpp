#include <iostream>

#include "Dijkstra.h"
#include "../Map/Map.h"

Dijkstra::Dijkstra(int width, int height) : width(width), height(height), infinity(std::numeric_limits<double>::infinity()) {}

// set AStar true for A* search, false for Dijkstra's search
// returns a vector of the path
std::vector<Vector2D> Dijkstra::a_star_search(
    Map& graph,
    Vector2D start,
    Vector2D goal,
    bool AStar,
    GameContext& ctx
)
{
    std::vector<Vector2D> cameFrom;
    std::vector<double> costSoFar;
    // Determine the total number of tiles in the map
    size_t gridSize = graph.get_map().size();
    // Resize vectors to match grid size
    // cameFrom[i] -> Stores which node led to this node
    // costSoFar[i] -> Stores the lowest cost found to reach this node
    cameFrom.resize(gridSize, Vector2D{ -1, -1 }); // Initialize with invalid positions
    costSoFar.resize(gridSize, infinity); // Initialize all costs as infinite

    // Priority queue for processing nodes, ordered by lowest cost (min-heap)
    std::priority_queue<FrontierNode, std::vector<FrontierNode>, std::greater<FrontierNode>> frontier;
    // Push the start node to the frontier
    frontier.emplace(start, 0);

    // Get the index of the starting position in the 1D vector representation of the grid
    int startIndex = graph.get_index(start);
    cameFrom.at(startIndex) = start; // The start node leads to itself
    costSoFar.at(startIndex) = 0; // Cost to reach the start is 0

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
            int currentIndex = graph.get_index(current); // Get index of the current node
            int nextIndex = graph.get_index(next); // Get index of the next node

            // If this new path is better (lower cost) than the previously known cost
            double new_cost = costSoFar.at(currentIndex) + graph.cost(current, next, ctx);

            if (costSoFar.at(nextIndex) == infinity || new_cost < costSoFar.at(nextIndex))
            {
                costSoFar.at(nextIndex) = new_cost; // Update cost to reach this node
                if (AStar) // for A* search
                {
                    double priority = new_cost + heuristic(next, goal);
                    frontier.emplace(next, priority);
                }
                else // for Dijkstra's search
                {
                    frontier.emplace(next, new_cost);
                }
                cameFrom.at(nextIndex) = current;
            }
        }

    }
    return reconstruct_path(start, goal, cameFrom);
}

std::vector<Vector2D> Dijkstra::reconstruct_path(
    Vector2D start, // The starting position
    Vector2D goal, // The goal position
    std::vector<Vector2D> cameFrom // Stores the previous node in the path
)
{
        std::vector<Vector2D> path; // Stores the reconstructed path
        Vector2D current = goal; // Start reconstruction from the goal

        // use labmda for index
		auto get_index = [this](Vector2D pos) { return pos.y * width + pos.x; };

        int goalIndex = get_index(goal);
        int startIndex = get_index(start);

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
