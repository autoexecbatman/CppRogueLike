// file: PathfindingTest.cpp
// The movement rule in Pathfinding: what a creature may enter, what that costs, the
// cost maps monsters descend, the A* route a mouse click walks, and the step every
// creature takes.
//
// The expectations come from the rule rather than a recorded run. A step onto a tile
// costs 1 and onto a shut door 2, diagonals cost what cardinals do in the cost maps, so
// the cheapest way between two open cells is max(|dx|, |dy|) steps. A creature with
// hands opens doors; one that cannot swim never enters water; a locked door and a wall
// stop everyone.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PathfindingTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <optional>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/Decoration.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/Pathfinding.h"
#include "src/SpellTile.h"
#include "src/Trap.h"
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

// A creature with what a move can touch: health for a trap, a dexterity for a web.
std::unique_ptr<Creature> make_creature(const char* name)
{
	auto creature = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, name, ColorPairId::WHITE_BLACK });
	creature->experienceReward = std::make_unique<ExperienceReward>(0);
	creature->healthPool = std::make_unique<HealthPool>(30);
	creature->armorClass = std::make_unique<ArmorClass>(10);
	creature->set_dexterity(10);
	return creature;
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
		ctx.traps = &traps;
		ctx.spellTiles = &spellTiles;
		ctx.decorations = &decorations;

		map.init_tiles();

		walker->set_body_plan(mock.body_plans.get("humanoid"));
		beast->set_body_plan(mock.body_plans.get("beast"));
		swimmer->set_body_plan(mock.body_plans.get("beast"));
		swimmer->add_state(ActorState::CAN_SWIM);
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

	// One row of floor from column 1 to 18, with a shut door at column 9.
	void corridor_with_door(int row)
	{
		open_room(Vector2D{ 1, row }, Vector2D{ 18, row });
		map.set_tile(Vector2D{ 9, row }, TileType::CLOSED_DOOR);
		map.tiles[map.get_index(Vector2D{ 9, row })].doorState = DoorState::CLOSED_UNLOCKED;
	}

	Pathfinding::Mobility mobility(const Creature& creature) const
	{
		return Pathfinding::mobility_of(creature, mock.tile_config);
	}

	std::vector<Vector2D> route(Vector2D start, Vector2D goal, const Creature& creature)
	{
		return pathfinder.find_route(start, goal, mobility(creature), ctx);
	}

	Map map{ MAP_SIDE, MAP_SIDE };
	MockGameContext mock{};
	GameContext ctx{};
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<std::unique_ptr<Trap>> traps{};
	std::vector<std::unique_ptr<SpellTile>> spellTiles{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	Pathfinding::Pathfinder pathfinder{};
	std::unique_ptr<Creature> walker{ make_creature("goblin") };
	std::unique_ptr<Creature> beast{ make_creature("wolf") };
	std::unique_ptr<Creature> swimmer{ make_creature("giant snake") };
};

// Hands open doors, swimming crosses water, and nothing crosses a wall.
TEST_F(PathfindingTest, MobilityFollowsTheBodyAndWhatItCanDo)
{
	const std::unique_ptr<Creature> rat = make_creature("rat");
	rat->set_body_plan(mock.body_plans.get("beast"));

	EXPECT_TRUE(mobility(*walker).contains(TileType::FLOOR));
	EXPECT_TRUE(mobility(*walker).contains(TileType::CLOSED_DOOR)) << "a humanoid has hands";
	EXPECT_FALSE(mobility(*beast).contains(TileType::CLOSED_DOOR)) << "a beast has no hands";
	EXPECT_FALSE(mobility(*walker).contains(TileType::WATER));
	EXPECT_TRUE(mobility(*swimmer).contains(TileType::WATER));
	EXPECT_FALSE(mobility(*swimmer).contains(TileType::WALL));
	EXPECT_FALSE(mobility(*walker) == mobility(*beast));
	EXPECT_TRUE(mobility(*beast) == mobility(*rat)) << "two beasts move alike";
}

// The price of one tile, for each kind of tile and mover.
TEST_F(PathfindingTest, StepCostPricesTheTileForTheMover)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 8, 8 });
	const Vector2D floorTile{ 2, 2 };
	const Vector2D door{ 3, 3 };
	const Vector2D lockedDoor{ 4, 4 };
	const Vector2D pool{ 5, 5 };
	const Vector2D barrelTile{ 6, 6 };
	map.set_tile(door, TileType::CLOSED_DOOR);
	map.tiles[map.get_index(door)].doorState = DoorState::CLOSED_UNLOCKED;
	map.set_tile(lockedDoor, TileType::CLOSED_DOOR);
	map.tiles[map.get_index(lockedDoor)].doorState = DoorState::CLOSED_LOCKED;
	map.set_tile(pool, TileType::WATER);
	decorations.push_back(std::make_unique<Decoration>(Decoration{ .position = barrelTile, .name = "barrel" }));

	EXPECT_EQ(Pathfinding::step_cost(floorTile, mobility(*walker), ctx), std::optional<int>{ 1 });
	EXPECT_EQ(Pathfinding::step_cost(door, mobility(*walker), ctx), std::optional<int>{ 2 });
	EXPECT_EQ(Pathfinding::step_cost(door, mobility(*beast), ctx), std::nullopt);
	EXPECT_EQ(Pathfinding::step_cost(lockedDoor, mobility(*walker), ctx), std::nullopt);
	EXPECT_EQ(Pathfinding::step_cost(pool, mobility(*walker), ctx), std::nullopt);
	EXPECT_EQ(Pathfinding::step_cost(pool, mobility(*swimmer), ctx), std::optional<int>{ 1 });
	EXPECT_EQ(Pathfinding::step_cost(barrelTile, mobility(*walker), ctx), std::nullopt);
	EXPECT_EQ(Pathfinding::step_cost(Vector2D{ 0, 0 }, mobility(*walker), ctx), std::nullopt) << "the border is wall";
	EXPECT_EQ(Pathfinding::step_cost(Vector2D{ -1, 3 }, mobility(*walker), ctx), std::nullopt) << "off the map";
}

// Nothing in the way, so the walk is the straight one and no longer.
TEST_F(PathfindingTest, AnOpenRunTakesTheShortestNumberOfSteps)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 10, 10 });
	const Vector2D start{ 1, 5 };
	const Vector2D goal{ 8, 5 };

	const std::vector<Vector2D> path = route(start, goal, *walker);

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

	const std::vector<Vector2D> path = route(start, goal, *walker);

	ASSERT_FALSE(path.empty());
	EXPECT_EQ(path.size(), static_cast<size_t>(chebyshev(start, goal)) + 1);
}

// Whatever route it picks, a walker has to be able to take every step of it.
TEST_F(PathfindingTest, EveryStepIsOntoAnAdjacentCellTheWalkerMayEnter)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 14, 14 });
	// A wall down the middle with one gap, so the route has to bend.
	for (int row = 1; row <= 12; ++row)
	{
		map.set_tile(Vector2D{ 7, row }, TileType::WALL);
	}

	const std::vector<Vector2D> path = route(Vector2D{ 3, 3 }, Vector2D{ 11, 3 }, *walker);

	ASSERT_FALSE(path.empty()) << "the gap below the wall was not found";
	for (size_t step = 1; step < path.size(); ++step)
	{
		EXPECT_EQ(chebyshev(path[step - 1], path[step]), 1)
			<< "step " << step << " jumps further than one cell";
		EXPECT_TRUE(Pathfinding::step_cost(path[step], mobility(*walker), ctx).has_value())
			<< "step " << step << " lands where the walker may not go";
	}
}

// A goal sealed off has no path, and that is reported as no path rather than as
// a route that stops short.
TEST_F(PathfindingTest, AGoalSealedBehindWallsHasNoPath)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 5, 5 });
	open_room(Vector2D{ 12, 12 }, Vector2D{ 15, 15 });

	EXPECT_TRUE(route(Vector2D{ 2, 2 }, Vector2D{ 14, 14 }, *walker).empty())
		<< "a route was returned across solid rock";
}

// Already there: the path is the cell the walker is on, not nothing.
TEST_F(PathfindingTest, StandingOnTheGoalIsAPathOfOneCell)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 5, 5 });
	const Vector2D here{ 3, 3 };

	const std::vector<Vector2D> path = route(here, here, *walker);

	ASSERT_EQ(path.size(), 1u);
	EXPECT_EQ(path.front(), here);
}

// A pool with a dry way round: the walker takes the long way, the swimmer the short one.
TEST_F(PathfindingTest, ARouteForANonSwimmerGoesRoundWater)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 14, 14 });
	// A wall of water down column 7 with dry ground only on the bottom row.
	for (int row = 1; row <= 13; ++row)
	{
		map.set_tile(Vector2D{ 7, row }, TileType::WATER);
	}
	const Vector2D start{ 3, 3 };
	const Vector2D goal{ 11, 3 };
	const auto is_wet = [this](Vector2D cell)
	{
		return map.is_water(cell);
	};

	const std::vector<Vector2D> dry = route(start, goal, *walker);
	const std::vector<Vector2D> wet = route(start, goal, *swimmer);

	ASSERT_FALSE(dry.empty()) << "the dry way round was not found";
	EXPECT_TRUE(std::ranges::none_of(dry, is_wet)) << "the non-swimmer's route wades";
	EXPECT_EQ(wet.size(), static_cast<size_t>(chebyshev(start, goal)) + 1) << "the swimmer went round";
}

// Water from wall to wall stops a non-swimmer's route outright.
TEST_F(PathfindingTest, WaterAcrossTheWholeWayLeavesNoRoute)
{
	open_room(Vector2D{ 1, 5 }, Vector2D{ 18, 5 });
	map.set_tile(Vector2D{ 9, 5 }, TileType::WATER);

	EXPECT_TRUE(route(Vector2D{ 2, 5 }, Vector2D{ 16, 5 }, *walker).empty());
	EXPECT_FALSE(route(Vector2D{ 2, 5 }, Vector2D{ 16, 5 }, *swimmer).empty());
}

// A shut door is a way through for hands and a wall for paws.
TEST_F(PathfindingTest, AShutDoorIsAWayThroughOnlyForHands)
{
	corridor_with_door(5);

	const std::vector<Vector2D> handed = route(Vector2D{ 2, 5 }, Vector2D{ 16, 5 }, *walker);

	ASSERT_FALSE(handed.empty()) << "the walker found no way through its own door";
	EXPECT_TRUE(std::ranges::contains(handed, Vector2D{ 9, 5 }));
	EXPECT_TRUE(route(Vector2D{ 2, 5 }, Vector2D{ 16, 5 }, *beast).empty()) << "a wolf opened a door";
}

// A locked door is never passed, but it can be walked up to: that is how the player
// reaches its lock.
TEST_F(PathfindingTest, ALockedDoorCanBeReachedButNotPassed)
{
	corridor_with_door(5);
	map.tiles[map.get_index(Vector2D{ 9, 5 })].doorState = DoorState::CLOSED_LOCKED;

	const std::vector<Vector2D> toTheDoor = route(Vector2D{ 2, 5 }, Vector2D{ 9, 5 }, *walker);

	ASSERT_FALSE(toTheDoor.empty()) << "the walk to the lock was refused";
	EXPECT_EQ(toTheDoor.back(), (Vector2D{ 9, 5 }));
	EXPECT_TRUE(route(Vector2D{ 2, 5 }, Vector2D{ 16, 5 }, *walker).empty()) << "a route ran through a lock";
}

// A barrel is never passed, but it can be walked up to: the last step is the bump that
// breaks it.
TEST_F(PathfindingTest, ABarrelCanBeReachedButNotPassed)
{
	open_room(Vector2D{ 1, 5 }, Vector2D{ 18, 5 });
	const Vector2D barrelTile{ 9, 5 };
	decorations.push_back(std::make_unique<Decoration>(Decoration{ .position = barrelTile, .name = "barrel" }));

	const std::vector<Vector2D> toTheBarrel = route(Vector2D{ 2, 5 }, barrelTile, *walker);

	ASSERT_FALSE(toTheBarrel.empty()) << "the walk to the barrel was refused";
	EXPECT_EQ(toTheBarrel.back(), barrelTile);
	EXPECT_TRUE(route(Vector2D{ 2, 5 }, Vector2D{ 16, 5 }, *walker).empty()) << "a route ran through a barrel";
}

// A cost is the price of the tiles walked onto: one a pace, two for the shut door.
TEST_F(PathfindingTest, CostToCountsPacesAndADoorCountsTwo)
{
	corridor_with_door(5);
	const Vector2D goal{ 2, 5 };

	EXPECT_EQ(pathfinder.cost_to(goal, goal, mobility(*walker), ctx), std::optional<int>{ 0 });
	EXPECT_EQ(pathfinder.cost_to(goal, Vector2D{ 8, 5 }, mobility(*walker), ctx), std::optional<int>{ 6 });
	EXPECT_EQ(pathfinder.cost_to(goal, Vector2D{ 12, 5 }, mobility(*walker), ctx), std::optional<int>{ 11 });
	EXPECT_EQ(pathfinder.cost_to(goal, Vector2D{ 12, 5 }, mobility(*beast), ctx), std::nullopt) << "the wolf has no way through";
	EXPECT_EQ(pathfinder.cost_to(goal, Vector2D{ 8, 5 }, mobility(*beast), ctx), std::optional<int>{ 6 });
}

// A map is reused while the clock stands still, and flooded again once it moves, so a
// door opened between two windows is seen in the next one.
TEST_F(PathfindingTest, ACostMapIsFloodedAgainWhenTheClockMoves)
{
	corridor_with_door(5);
	const Vector2D goal{ 2, 5 };
	const Vector2D beyond{ 12, 5 };
	ASSERT_EQ(pathfinder.cost_to(goal, beyond, mobility(*walker), ctx), std::optional<int>{ 11 });

	map.set_tile(Vector2D{ 9, 5 }, TileType::OPEN_DOOR);
	EXPECT_EQ(pathfinder.cost_to(goal, beyond, mobility(*walker), ctx), std::optional<int>{ 11 })
		<< "the map was flooded again inside one window";

	mock.game_state.set_time(mock.game_state.get_time() + 1);
	EXPECT_EQ(pathfinder.cost_to(goal, beyond, mobility(*walker), ctx), std::optional<int>{ 10 });
}

// A different goal is a different map, whatever the clock says.
TEST_F(PathfindingTest, ACostMapFollowsItsGoal)
{
	open_room(Vector2D{ 1, 5 }, Vector2D{ 18, 5 });

	EXPECT_EQ(pathfinder.cost_to(Vector2D{ 2, 5 }, Vector2D{ 10, 5 }, mobility(*walker), ctx), std::optional<int>{ 8 });
	EXPECT_EQ(pathfinder.cost_to(Vector2D{ 14, 5 }, Vector2D{ 10, 5 }, mobility(*walker), ctx), std::optional<int>{ 4 });
}

// A step onto a shut door opens it and the opener stays; the next step goes through.
TEST_F(PathfindingTest, AStepOpensAShutDoorAndTheNextGoesThrough)
{
	corridor_with_door(5);
	walker->position = Vector2D{ 8, 5 };

	EXPECT_EQ(Pathfinding::step(*walker, Vector2D{ 9, 5 }, ctx), Pathfinding::StepResult::OPENED_DOOR);
	EXPECT_EQ(walker->position, (Vector2D{ 8, 5 })) << "opening a door moved the opener";
	EXPECT_TRUE(map.is_open_door(Vector2D{ 9, 5 }));
	EXPECT_EQ(Pathfinding::step(*walker, Vector2D{ 9, 5 }, ctx), Pathfinding::StepResult::MOVED);
	EXPECT_EQ(walker->position, (Vector2D{ 9, 5 }));
}

// A wolf at a shut door, a walker at water and anyone at an occupied tile stay put.
TEST_F(PathfindingTest, AStepIsRefusedWhereTheMoverMayNotGo)
{
	corridor_with_door(5);
	map.set_tile(Vector2D{ 4, 5 }, TileType::WATER);
	beast->position = Vector2D{ 8, 5 };
	walker->position = Vector2D{ 3, 5 };
	creatures.push_back(make_creature("orc"));
	creatures.back()->position = Vector2D{ 2, 5 };

	EXPECT_EQ(Pathfinding::step(*beast, Vector2D{ 9, 5 }, ctx), Pathfinding::StepResult::REFUSED);
	EXPECT_FALSE(map.is_open_door(Vector2D{ 9, 5 })) << "a wolf opened the door";
	EXPECT_EQ(Pathfinding::step(*walker, Vector2D{ 4, 5 }, ctx), Pathfinding::StepResult::REFUSED);
	EXPECT_EQ(Pathfinding::step(*walker, Vector2D{ 2, 5 }, ctx), Pathfinding::StepResult::REFUSED);
	EXPECT_EQ(walker->position, (Vector2D{ 3, 5 }));
	EXPECT_EQ(beast->position, (Vector2D{ 8, 5 }));
}

// A monster walks into traps: a hidden one springs on it with no roll to notice it, the
// damage lands, and the step stops short.
TEST_F(PathfindingTest, AMonsterSteppingOntoAHiddenTrapSpringsIt)
{
	open_room(Vector2D{ 1, 5 }, Vector2D{ 18, 5 });
	traps.push_back(std::make_unique<Trap>(Vector2D{ 6, 5 }, TrapType::DART, mock.tile_config));
	walker->position = Vector2D{ 5, 5 };
	const int healthBefore = walker->get_hp();
	mock.dice.set_next_roll(3); // the dart's 1d4
	mock.dice.set_next_roll(2); // the trap survives its springing

	EXPECT_EQ(Pathfinding::step(*walker, Vector2D{ 6, 5 }, ctx), Pathfinding::StepResult::STOPPED);
	EXPECT_EQ(traps.front()->get_state(), TrapState::TRIGGERED);
	EXPECT_EQ(walker->get_hp(), healthBefore - 3);
	EXPECT_EQ(walker->position, (Vector2D{ 5, 5 }));
}

// A creature standing in a one-wide corridor closes it to a route, though not to a route
// that ends on it.
TEST_F(PathfindingTest, ARouteDoesNotRunThroughACreature)
{
	open_room(Vector2D{ 1, 5 }, Vector2D{ 18, 5 });
	creatures.push_back(make_creature("orc"));
	creatures.back()->position = Vector2D{ 9, 5 };

	EXPECT_TRUE(route(Vector2D{ 2, 5 }, Vector2D{ 16, 5 }, *walker).empty()) << "the route ran through the orc";
	const std::vector<Vector2D> toTheOrc = route(Vector2D{ 2, 5 }, Vector2D{ 9, 5 }, *walker);
	ASSERT_FALSE(toTheOrc.empty()) << "a creature standing on the goal hid the goal";
	EXPECT_EQ(toTheOrc.back(), (Vector2D{ 9, 5 }));
}

// can_enter answers for standing there now: a shut door has to be opened first.
TEST_F(PathfindingTest, CanEnterIsForStandingThereNow)
{
	corridor_with_door(5);
	map.set_tile(Vector2D{ 4, 5 }, TileType::WATER);

	EXPECT_TRUE(Pathfinding::can_enter(*walker, Vector2D{ 6, 5 }, ctx));
	EXPECT_FALSE(Pathfinding::can_enter(*walker, Vector2D{ 9, 5 }, ctx));
	EXPECT_FALSE(Pathfinding::can_enter(*walker, Vector2D{ 4, 5 }, ctx));
	EXPECT_TRUE(Pathfinding::can_enter(*swimmer, Vector2D{ 4, 5 }, ctx));
}

// end of file: PathfindingTest.cpp
