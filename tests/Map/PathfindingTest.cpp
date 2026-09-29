// file: PathfindingTest.cpp
// A* over the map, which is how a mouse click walks the player somewhere.
//
// Dijkstra::a_star_search is reached from one place in the game,
// PlayerController's walk-to-destination, and had no test at all. What it
// promises: a path from start to goal inclusive, every step to an
// eight-adjacent cell the walker may stand on, no longer than the Chebyshev
// distance when nothing is in the way, and empty when the goal cannot be
// reached.
//
// The expectations here come from that rule rather than from a recorded run.
// With eight-way movement and diagonals costing what cardinals do, the shortest
// route between two open cells is max(|dx|, |dy|) steps, so the path is one cell
// longer than that.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PathfindingTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "src/Creature.h"
#include "src/Dijkstra.h"
#include "src/Map.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int MAP_SIDE = 20;

// The number of eight-way steps between two cells with nothing in the way.
int chebyshev(Vector2D from, Vector2D to)
{
	return std::max(std::abs(from.x - to.x), std::abs(from.y - to.y));
}
} // namespace

class PathfindingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.map = &map;
		ctx.creatures = &creatures;

		map.init_tiles();
	}

	// Opens a rectangle of floor, walls everywhere else.
	void open_room(Vector2D topLeft, Vector2D bottomRight)
	{
		for (int row = topLeft.y; row <= bottomRight.y; ++row)
		{
			for (int col = topLeft.x; col <= bottomRight.x; ++col)
			{
				map.set_tile(Vector2D{ col, row }, TileType::FLOOR, 1.0);
			}
		}
	}

	std::vector<Vector2D> path_between(Vector2D start, Vector2D goal)
	{
		return pathfinder.a_star_search(map, start, goal, true, ctx);
	}

	Map map{ MAP_SIDE, MAP_SIDE };
	MockGameContext mock{};
	GameContext ctx{};
	std::vector<std::unique_ptr<Creature>> creatures;
	Dijkstra pathfinder{ MAP_SIDE, MAP_SIDE };
};

// Nothing in the way, so the walk is the straight one and no longer.
TEST_F(PathfindingTest, AnOpenRunTakesTheShortestNumberOfSteps)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 10, 10 });
	const Vector2D start{ 1, 5 };
	const Vector2D goal{ 8, 5 };

	const std::vector<Vector2D> path = path_between(start, goal);

	ASSERT_FALSE(path.empty()) << "no path across open floor";
	EXPECT_EQ(path.front(), start) << "the path does not begin where the walker is";
	EXPECT_EQ(path.back(), goal) << "the path does not end at the goal";
	EXPECT_EQ(path.size(), static_cast<size_t>(chebyshev(start, goal)) + 1) << "the walk wandered";
}

// A diagonal costs what a cardinal step costs, so crossing a room corner to
// corner is as many steps as crossing one side of it.
TEST_F(PathfindingTest, ADiagonalRunIsNoLongerThanAStraightOne)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 10, 10 });
	const Vector2D start{ 2, 2 };
	const Vector2D goal{ 7, 7 };

	const std::vector<Vector2D> path = path_between(start, goal);

	ASSERT_FALSE(path.empty());
	EXPECT_EQ(path.size(), static_cast<size_t>(chebyshev(start, goal)) + 1);
}

// Whatever route it picks, a walker has to be able to take every step of it.
TEST_F(PathfindingTest, EveryStepIsOntoAnAdjacentCellThatCanBeWalked)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 14, 14 });
	// A wall down the middle with one gap, so the route has to bend.
	for (int row = 1; row <= 12; ++row)
	{
		map.set_tile(Vector2D{ 7, row }, TileType::WALL, 0.0);
	}

	const std::vector<Vector2D> path = path_between(Vector2D{ 3, 3 }, Vector2D{ 11, 3 });

	ASSERT_FALSE(path.empty()) << "the gap below the wall was not found";
	for (size_t step = 1; step < path.size(); ++step)
	{
		EXPECT_EQ(chebyshev(path[step - 1], path[step]), 1)
			<< "step " << step << " jumps further than one cell";
		EXPECT_TRUE(map.can_walk(path[step], ctx))
			<< "step " << step << " lands where the walker cannot stand";
	}
}

// A goal sealed off has no path, and that is reported as no path rather than as
// a route that stops short.
TEST_F(PathfindingTest, AGoalSealedBehindWallsHasNoPath)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 5, 5 });
	open_room(Vector2D{ 12, 12 }, Vector2D{ 15, 15 });

	EXPECT_TRUE(path_between(Vector2D{ 2, 2 }, Vector2D{ 14, 14 }).empty())
		<< "a route was returned across solid rock";
}

// Already there: the path is the cell the walker is on, not nothing.
TEST_F(PathfindingTest, StandingOnTheGoalIsAPathOfOneCell)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 5, 5 });
	const Vector2D here{ 3, 3 };

	const std::vector<Vector2D> path = path_between(here, here);

	ASSERT_EQ(path.size(), 1u);
	EXPECT_EQ(path.front(), here);
}

// end of file: PathfindingTest.cpp
