// file: MonsterMovementTest.cpp
// A hunting monster moves by the one rule Pathfinding holds: a creature with hands opens
// a shut door on its way, one without waits at it, nobody passes a lock, a trap on the
// way springs on whoever walks in, and a non-swimmer takes the long way round water.
//
// These drive AiMonster's hunting step rather than the predicates, because the defects
// this replaces were never in a rule - they were in which rule a mover asked. The step is
// reached through a subclass that makes it public, so a turn does not depend on the
// awareness dice: a shut door blocks sight, and an unseen monster hunts only on a d6.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterMovementTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <vector>

#include "src/AiMonster.h"
#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/MonsterAttacker.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/Trap.h"
#include "tests/mocks/TestDice.h"

namespace
{

constexpr int STARTING_HP = 100;
constexpr int CORRIDOR_ROW = 5;
constexpr Vector2D PLAYER_TILE{ 3, CORRIDOR_ROW };
constexpr Vector2D DOOR_TILE{ 7, CORRIDOR_ROW };
// The tile on the monster's side of the door.
constexpr Vector2D NEAR_SIDE_OF_DOOR{ 8, CORRIDOR_ROW };
constexpr Vector2D MONSTER_START{ 12, CORRIDOR_ROW };

// Many more turns than the walk needs, so a monster that has not arrived has stopped.
constexpr int ROUNDS = 30;

// The hunting step, public for the test.
class HuntingAi final : public AiMonster
{
public:
	using AiMonster::move_or_attack;
};

} // namespace

class MonsterMovementTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);
		game.bodyPlanRegistry.load(Paths::BODY_PLANS);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_dr(0);
		player->set_thaco(20);
		player->set_strength(10);
		player->set_dexterity(10);

		monster.experienceReward = std::make_unique<ExperienceReward>(0);
		monster.healthPool = std::make_unique<HealthPool>(STARTING_HP);
		monster.armorClass = std::make_unique<ArmorClass>(10);
		monster.attacker = std::make_unique<MonsterAttacker>(
			monster,
			DamageInfo{ "1d4", DamageType::PHYSICAL });
		monster.set_dr(0);
		monster.set_thaco(20);
		monster.set_strength(8);
		monster.set_dexterity(10);
		monster.set_natural_attack("bite");
		monster.set_body_plan(game.bodyPlanRegistry.get("humanoid"));

		ctx = game.context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		ctx.creatures = &creatures;

		map.init_tiles();
		for (int column = 1; column < 19; ++column)
		{
			map.set_tile(Vector2D{ column, CORRIDOR_ROW }, TileType::FLOOR);
		}
	}

	void shut_the_door(DoorState doorState)
	{
		map.set_tile(DOOR_TILE, TileType::CLOSED_DOOR);
		map.tiles[map.get_index(DOOR_TILE)].doorState = doorState;
	}

	void open_room(Vector2D topLeft, Vector2D bottomRight)
	{
		for (int row = topLeft.y; row <= bottomRight.y; ++row)
		{
			for (int column = topLeft.x; column <= bottomRight.x; ++column)
			{
				map.set_tile(Vector2D{ column, row }, TileType::FLOOR);
			}
		}
	}

	// Creatures that stand where they are put and never take a turn.
	void place_blockers(std::initializer_list<Vector2D> tiles)
	{
		for (const Vector2D& tile : tiles)
		{
			auto blocker = std::make_unique<Creature>(
				tile,
				ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK });
			blocker->healthPool = std::make_unique<HealthPool>(STARTING_HP);
			creatures.push_back(std::move(blocker));
		}
	}

	// Each round is its own window on the clock, as in play, so a cost map flooded
	// before a door opened is not the one read after it.
	void hunt(int rounds)
	{
		for (int round = 0; round < rounds; ++round)
		{
			hunter->move_or_attack(monster, *player, ctx);
			game.gameState.advance_clock(1);
		}
	}

	Game game{ TEST_DICE_SEED };
	GameContext ctx{};
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(PLAYER_TILE) };
	std::vector<std::unique_ptr<Creature>> creatures{};
	Creature monster{
		MONSTER_START,
		ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK },
	};
	HuntingAi* hunter{ nullptr };

	void TearDown() override { game.dice.clear_fixed_rolls(); }

public:
	MonsterMovementTest()
	{
		auto ai = std::make_unique<HuntingAi>();
		hunter = ai.get();
		monster.ai = std::move(ai);
	}
};

// Hands open a shut door, and the hunt goes on through it.
TEST_F(MonsterMovementTest, AGoblinOpensAShutDoorOnItsWayToThePlayer)
{
	shut_the_door(DoorState::CLOSED_UNLOCKED);

	hunt(ROUNDS);

	EXPECT_TRUE(map.is_open_door(DOOR_TILE))
		<< "the goblin never opened the door";
	EXPECT_LE(monster.position.chebyshev_distance_to(PLAYER_TILE), 1)
		<< "the goblin did not reach the player";
}

// Paws do not: the wolf comes up to the door and waits there.
TEST_F(MonsterMovementTest, AWolfWaitsAtAShutDoor)
{
	monster.set_body_plan(game.bodyPlanRegistry.get("beast"));
	shut_the_door(DoorState::CLOSED_UNLOCKED);

	hunt(ROUNDS);

	EXPECT_FALSE(map.is_open_door(DOOR_TILE)) << "a wolf opened a door";
	EXPECT_EQ(monster.position, NEAR_SIDE_OF_DOOR)
		<< "the wolf should wait against the door";
}

// A lock stops hands as well.
TEST_F(MonsterMovementTest, NobodyPassesALockedDoor)
{
	shut_the_door(DoorState::CLOSED_LOCKED);

	hunt(ROUNDS);

	EXPECT_FALSE(map.is_open_door(DOOR_TILE))
		<< "a goblin opened a locked door";
	EXPECT_EQ(monster.position, NEAR_SIDE_OF_DOOR);
}

// A monster does not route round a trap it walks into: the trap springs on it.
TEST_F(MonsterMovementTest, AMonsterWalksIntoATrapOnItsWay)
{
	game.traps.push_back(
		std::make_unique<Trap>(
			Vector2D{ 9, CORRIDOR_ROW },
			TrapType::DART,
			game.tileConfig));
	const int healthBefore = monster.get_hp();

	hunt(ROUNDS);

	EXPECT_NE(game.traps.front()->get_state(), TrapState::HIDDEN)
		<< "the trap never went off";
	EXPECT_LT(monster.get_hp(), healthBefore) << "the dart missed nobody";
}

// A wall of water with a dry way round that is longer than the wet way: a non-swimmer
// still arrives, and never wades.
TEST_F(MonsterMovementTest, ANonSwimmerTakesTheLongWayRoundWater)
{
	for (int row = 1; row <= 9; ++row)
	{
		for (int column = 1; column < 19; ++column)
		{
			map.set_tile(Vector2D{ column, row }, TileType::FLOOR);
		}
	}
	for (int row = 1; row <= 8; ++row)
	{
		for (int column = 7; column <= 9; ++column)
		{
			map.set_tile(Vector2D{ column, row }, TileType::WATER);
		}
	}
	player->position = Vector2D{ 3, 2 };
	monster.position = Vector2D{ 14, 2 };

	for (int round = 0; round < ROUNDS; ++round)
	{
		hunt(1);
		ASSERT_FALSE(map.is_water(monster.position))
			<< "the goblin waded at round " << round;
	}

	EXPECT_LE(monster.position.chebyshev_distance_to(player->position), 1)
		<< "the goblin never came round the water";
}

// A crowd: the tiles straight ahead are taken, so the hunter goes round it, with room on
// both sides.
TEST_F(MonsterMovementTest, AHunterSidestepsRoundCreaturesInItsWay)
{
	open_room(Vector2D{ 1, 2 }, Vector2D{ 12, 6 });
	player->position = Vector2D{ 2, 4 };
	monster.position = Vector2D{ 7, 4 };
	place_blockers({ Vector2D{ 6, 3 }, Vector2D{ 6, 4 }, Vector2D{ 6, 5 } });

	hunt(ROUNDS);

	EXPECT_LE(monster.position.chebyshev_distance_to(player->position), 1)
		<< "the hunter stayed behind the crowd";
}

// The crowd leaves a dead end on one side and a way round on the other. A sidestep into
// the dead end leads back to where the hunter stood, so it must take the open side.
TEST_F(MonsterMovementTest, AHunterTakesTheSideOfACrowdThatLeadsSomewhere)
{
	open_room(Vector2D{ 1, 3 }, Vector2D{ 12, 6 });
	player->position = Vector2D{ 2, 4 };
	monster.position = Vector2D{ 7, 4 };
	place_blockers({ Vector2D{ 6, 3 }, Vector2D{ 6, 4 }, Vector2D{ 6, 5 } });

	hunt(ROUNDS);

	EXPECT_LE(monster.position.chebyshev_distance_to(player->position), 1)
		<< "the hunter paced at the dead end";
}

// A wall of creatures with one gap at the far end: every tile along it is as far from the
// player as the next, and the hunter walks its length to the gap.
TEST_F(MonsterMovementTest, AHunterWalksTheLengthOfAWideCrowdToItsGap)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 12, 8 });
	player->position = Vector2D{ 2, 4 };
	monster.position = Vector2D{ 7, 4 };
	place_blockers(
		{
			Vector2D{ 6, 1 },
			Vector2D{ 6, 2 },
			Vector2D{ 6, 3 },
			Vector2D{ 6, 4 },
			Vector2D{ 6, 5 },
			Vector2D{ 6, 6 },
			Vector2D{ 6, 7 },
		});

	hunt(ROUNDS);

	EXPECT_LE(monster.position.chebyshev_distance_to(player->position), 1)
		<< "the hunter never found the gap";
}

// Hemmed in on every side that is no further from the player, the hunter backs out and
// goes round, because the way round is a route and backing out is its first step.
TEST_F(MonsterMovementTest, AHunterBoxedInBacksOutAndGoesRound)
{
	open_room(Vector2D{ 1, 3 }, Vector2D{ 12, 6 });
	player->position = Vector2D{ 2, 4 };
	monster.position = Vector2D{ 7, 4 };
	place_blockers(
		{
			Vector2D{ 6, 3 },
			Vector2D{ 6, 4 },
			Vector2D{ 6, 5 },
			Vector2D{ 7, 3 },
			Vector2D{ 7, 5 },
		});

	hunt(ROUNDS);

	EXPECT_LE(monster.position.chebyshev_distance_to(player->position), 1)
		<< "the hunter stayed boxed in";
}

// The player stands inside a closed ring of creatures, so no route reaches it. The hunter
// comes up to the ring, going past a creature on its straight line in, and then holds:
// every tile beside it is either taken or no nearer, and a step to one of those would
// only be undone by the next.
TEST_F(MonsterMovementTest, AHunterComesUpToAClosedRingAndHolds)
{
	open_room(Vector2D{ 1, 1 }, Vector2D{ 12, 8 });
	player->position = Vector2D{ 5, 4 };
	place_blockers(
		{
			Vector2D{ 4, 3 },
			Vector2D{ 5, 3 },
			Vector2D{ 6, 3 },
			Vector2D{ 4, 4 },
			Vector2D{ 6, 4 },
			Vector2D{ 4, 5 },
			Vector2D{ 5, 5 },
			Vector2D{ 6, 5 },
		});
	// On the hunter's straight line in.
	place_blockers({ Vector2D{ 5, 6 } });
	monster.position = Vector2D{ 5, 7 };

	hunt(1);
	const Vector2D atTheRing = monster.position;
	ASSERT_EQ(atTheRing.chebyshev_distance_to(player->position), 2)
		<< "the hunter did not come up to the ring";

	// Every round, because a pace of two steps ends where it began.
	for (int round = 0; round < ROUNDS; ++round)
	{
		hunt(1);
		ASSERT_EQ(monster.position, atTheRing)
			<< "the hunter paced along the ring at round " << round;
	}
}

// In a corridor with a creature between it and the player there is no way round, so the
// hunter comes up behind the creature and waits there.
TEST_F(MonsterMovementTest, AHunterBehindACreatureInACorridorWaits)
{
	place_blockers({ Vector2D{ 4, CORRIDOR_ROW } });
	monster.position = Vector2D{ 7, CORRIDOR_ROW };

	hunt(ROUNDS);

	EXPECT_EQ(monster.position, (Vector2D{ 5, CORRIDOR_ROW }))
		<< "the hunter should wait behind the creature";
}

// end of file: MonsterMovementTest.cpp
