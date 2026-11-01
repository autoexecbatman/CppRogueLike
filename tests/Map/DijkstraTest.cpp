// file: DijkstraTest.cpp
// The search behind Pathfinding's cost maps and routes, driven through
// Pathfinder and on its own.
//
// Where several routes cost the same, the one taken is decided by the order the
// frontier pops ties in, which no rule states. It is pinned as the route the
// search took before the class was restored from 8b173641^, so a change to the
// frontier or to the neighbour order moves it. The class's own cases use a
// corridor of five floor tiles, (1,1) to (5,1), on a 7 by 3 grid, every step
// costing 1: the examples in Dijkstra.h are these calls.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DijkstraTest.*

#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "src/Dijkstra.h"
#include "src/Map.h"
#include "src/Pathfinding.h"
#include "src/TileType.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace CppRL
{

	namespace
	{
		constexpr int MAP_SIDE = 20;
		constexpr int CORRIDOR_WIDTH = 7;
		constexpr int CORRIDOR_HEIGHT = 3;
		constexpr int CORRIDOR_ROW = 1;
		constexpr int CORRIDOR_FIRST = 1;
		constexpr int CORRIDOR_LAST = 5;

		// The corridor's two questions: the floor tiles either side, and a
		// price of 1 for every step.
		SearchGraph corridor_graph()
		{
			const auto along_the_row = [](Vector2D current)
			{
				std::vector<Vector2D> open{};
				for (const Vector2D next : { current + DIR_W, current + DIR_E })
				{
					if (next.y == CORRIDOR_ROW && next.x >= CORRIDOR_FIRST &&
						next.x <= CORRIDOR_LAST)
					{
						open.push_back(next);
					}
				}
				return open;
			};
			// Neighbours touch, so the distance between them is 1.
			const auto one_per_step = [](Vector2D current, Vector2D next)
			{
				return static_cast<double>(current.chebyshev_distance_to(next));
			};
			return SearchGraph{
				.neighbours = along_the_row,
				.cost = one_per_step,
			};
		}
	} // namespace

	class DijkstraTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ctx = mock.to_game_context();
			ctx.map = &map;
			map.init_tiles();
		}

		// Opens a rectangle of floor, walls everywhere else.
		void open_room(Vector2D topLeft, Vector2D bottomRight)
		{
			for (int row = topLeft.y; row <= bottomRight.y; ++row)
			{
				for (int col = topLeft.x; col <= bottomRight.x; ++col)
				{
					map.set_tile(Vector2D{ col, row }, TileType::FLOOR);
				}
			}
		}

		Map map{ MAP_SIDE, MAP_SIDE };
		MockGameContext mock{};
		GameContext ctx{};
		Pathfinding::Pathfinder pathfinder{};
		Dijkstra search{ CORRIDOR_WIDTH, CORRIDOR_HEIGHT };
	};

	// Six steps, three of them diagonal, can be taken in many orders at one
	// cost; the search takes this one.
	TEST_F(DijkstraTest, ATieBetweenEqualRoutesIsBrokenTheWayItWasBefore)
	{
		open_room(Vector2D{ 1, 1 }, Vector2D{ 10, 10 });
		const Pathfinding::Mobility walker{ { TileType::FLOOR } };

		const std::vector<Vector2D> route = pathfinder.find_route(
			Vector2D{ 2, 2 },
			Vector2D{ 8, 5 },
			walker,
			ctx);

		// The cardinals first, then the diagonals.
		const std::vector<Vector2D> taken{
			Vector2D{ 2, 2 },
			Vector2D{ 3, 2 },
			Vector2D{ 4, 2 },
			Vector2D{ 5, 2 },
			Vector2D{ 6, 3 },
			Vector2D{ 7, 4 },
			Vector2D{ 8, 5 },
		};
		EXPECT_EQ(route, taken);
	}

	// Start and goal are both in the path, start first.
	TEST_F(DijkstraTest, AnAStarSearchReturnsThePathFromStartToGoal)
	{
		const std::vector<Vector2D> path = search.a_star_search(
			corridor_graph(),
			Vector2D{ 1, 1 },
			Vector2D{ 4, 1 },
			SearchOrder::A_STAR);

		const std::vector<Vector2D> expected{
			Vector2D{ 1, 1 },
			Vector2D{ 2, 1 },
			Vector2D{ 3, 1 },
			Vector2D{ 4, 1 },
		};
		EXPECT_EQ(path, expected);
		EXPECT_EQ(
			search.reconstruct_path(Vector2D{ 1, 1 }, Vector2D{ 4, 1 }),
			expected);
	}

	// (6,1) is past the corridor's end, so nothing leads to it.
	TEST_F(DijkstraTest, AGoalTheSearchNeverReachesHasNoPath)
	{
		const std::vector<Vector2D> path = search.a_star_search(
			corridor_graph(),
			Vector2D{ 1, 1 },
			Vector2D{ 6, 1 },
			SearchOrder::A_STAR);

		EXPECT_TRUE(path.empty());
		EXPECT_TRUE(search.reconstruct_path(Vector2D{ 1, 1 }, Vector2D{ 6, 1 })
				.empty());
	}

	// With no goal the search prices every tile it can reach and draws no path.
	TEST_F(DijkstraTest, AFloodWithNoGoalPricesEveryTileItReaches)
	{
		const std::vector<Vector2D> path = search.a_star_search(
			corridor_graph(),
			Vector2D{ 1, 1 },
			std::nullopt,
			SearchOrder::DIJKSTRA);

		EXPECT_TRUE(path.empty());
		EXPECT_EQ(
			search.cost_so_far(Vector2D{ 1, 1 }),
			std::optional<double>{ 0.0 });
		EXPECT_EQ(
			search.cost_so_far(Vector2D{ 5, 1 }),
			std::optional<double>{ 4.0 });
		EXPECT_EQ(search.cost_so_far(Vector2D{ 6, 1 }), std::nullopt);
		EXPECT_EQ(search.cost_so_far(Vector2D{ 1, 2 }), std::nullopt);
	}

	// The heuristic is the number of eight-way steps with nothing in the way.
	TEST_F(DijkstraTest, TheHeuristicIsTheChebyshevDistance)
	{
		EXPECT_EQ(search.heuristic(Vector2D{ 2, 2 }, Vector2D{ 8, 5 }), 6.0);
		EXPECT_EQ(search.heuristic(Vector2D{ 4, 1 }, Vector2D{ 4, 1 }), 0.0);
	}

} // namespace CppRL
