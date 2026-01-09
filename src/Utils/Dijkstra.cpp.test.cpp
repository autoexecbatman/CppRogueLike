#include <cassert>
#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include "Dijkstra.h"
#include "../Map/Map.h"
#include <gtest/gtest.h> // Assuming Google Test is used for this unit test

// Mocking Vector2D and Map classes for testing purposes
class Vector2D {
public:
    int x, y;
    Vector2D(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Vector2D& other) const { return x == other.x && y == other.y; }
};

class Map {
public:
    std::vector<Vector2D> map;
    int width, height;
    Map(int width, int height) : width(width), height(height) {}
    std::vector<Vector2D> get_map() const { return map; }
    int get_index(const Vector2D& pos) const { return pos.y * width + pos.x; }
    double cost(const Vector2D& from, const Vector2D& to) const { return 1.0; } // Assuming all movements have the same cost
    std::vector<Vector2D> neighbors(const Vector2D& node) const {
        std::vector<Vector2D> neighbors;
        if (node.x > 0) neighbors.push_back({node.x - 1, node.y});
        if (node.x < width - 1) neighbors.push_back({node.x + 1, node.y});
        if (node.y > 0) neighbors.push_back({node.x, node.y - 1});
        if (node.y < height - 1) neighbors.push_back({node.x, node.y + 1});
        return neighbors;
    }
};

// Mocking FrontierNode for testing purposes
class FrontierNode {
public:
    Vector2D vertex;
    double cost;
    FrontierNode(Vector2D v, double c) : vertex(v), cost(c) {}
    bool operator>(const FrontierNode& other) const { return cost > other.cost; }
};

// Mocking heuristic function for testing purposes
double heuristic(const Vector2D& a, const Vector2D& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y); // Manhattan distance
}

TEST(DijkstraTest, BasicSearchAndPathReconstruction) {
    Map map(5, 5);
    Dijkstra dijkstra(5, 5);
    Vector2D start(0, 0);
    Vector2D goal(4, 4);

    std::vector<Vector2D> path = dijkstra.a_star_search(map, start, goal, true);

    // Expected path from (0,0) to (4,4) is a straight line: (0,0), (1,1), ..., (4,4)
    std::vector<Vector2D> expectedPath = { {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4} };
    EXPECT_EQ(path, expectedPath);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}