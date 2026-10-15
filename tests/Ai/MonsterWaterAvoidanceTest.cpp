// file: MonsterWaterAvoidanceTest.cpp
// A monster that cannot swim does not walk into water, and one that can still crosses.
//
// What this is for, and why a passing WaterMovementTest did not cover it. The rule
// lived in Map::is_collision, which read blocks_movement and bypass_state out of
// tile_config.json and was correct, and whose only caller was PlayerController. So the
// rule reached the player and nothing else: every monster moved through the can_walk
// overload that takes no creature, which checks walls, shut doors, actors and
// decorations and never opens the tile definition. Water is walkable in the
// field-of-view grid, so that overload called it passable for everybody, and all 43
// monsters in data/content/monsters.json have canSwim false. is_collision is gone now
// and its rule lives in can_walk(mover, pos, ctx), which is what every mover asks.
//
// These tests drive AiMonster::update rather than a predicate, because the defect was
// never in the rule - it was in which gate the movement asks.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterWaterAvoidanceTest.*

#include <gtest/gtest.h>

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
#include "src/RandomDice.h"

namespace
{

constexpr int STARTING_HP = 100;

// One straight corridor with a single water tile in it. The player stands beyond the
// water and the goblin starts on the dry side, so closing the distance means the flow
// field's best next step is the water itself - which is the move that has to be refused.
constexpr int CORRIDOR_ROW = 5;
constexpr int PLAYER_COLUMN = 4;
constexpr int WATER_COLUMN = 6;
constexpr int BANK_COLUMN = 7; // the dry tile the goblin should stop on
// Four tiles from the player, which is the unlit sight radius, so the goblin stands in
// the player's field of view and update_awareness books it aware on its own turn. An
// aware monster hunts with no die in the way, so nothing here depends on scripted rolls.
constexpr int GOBLIN_START_COLUMN = 8;

// Far more turns than the one step to the bank needs, so a goblin that stops has
// demonstrably stopped rather than been slow.
constexpr int ROUNDS = 12;

} // namespace

class MonsterWaterAvoidanceTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_dr(0);
		player->set_thaco(20);
		player->set_strength(10);
		player->set_dexterity(10);

		goblin.experienceReward = std::make_unique<ExperienceReward>(0);
		goblin.healthPool = std::make_unique<HealthPool>(STARTING_HP);
		goblin.armorClass = std::make_unique<ArmorClass>(10);
		goblin.attacker = std::make_unique<MonsterAttacker>(goblin, DamageInfo{ "1d4", DamageType::PHYSICAL });
		goblin.set_dr(0);
		goblin.set_thaco(20);
		goblin.set_strength(8);
		goblin.set_dexterity(10);
		goblin.set_natural_attack("bite");
		goblin.ai = std::make_unique<AiMonster>();

		ctx = game.context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		ctx.creatures = &creatures;

		map.init_tiles();
		for (int column = 1; column < 19; ++column)
		{
			map.set_tile(Vector2D{ column, CORRIDOR_ROW }, TileType::FLOOR, 1.0);
		}
		map.set_tile(Vector2D{ WATER_COLUMN, CORRIDOR_ROW }, TileType::WATER, 10.0);
		map.compute_fov(ctx);
		map.rebuild_dijkstra_map({ player->position }, ctx);

		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// No rolls are queued on purpose. An aware monster above half health takes the
	// hunting branch, which reads no die, and check_morale returns before its 2d10 while
	// hp*2 > hpMax. A round that consumed a roll here would fall through to real dice,
	// so the absence of a queue is also what would expose one.
	// Every tile the goblin stood on, in order, including where it started. Checking
	// only the final position is not enough: without the fix the goblin crosses the
	// water and ends dry on the player's side, which reads as never having been wet.
	std::vector<Vector2D> walk_the_goblin()
	{
		std::vector<Vector2D> visited{ goblin.position };
		for (int round = 0; round < ROUNDS; ++round)
		{
			goblin.ai->update(goblin, ctx);
			visited.push_back(goblin.position);
		}
		return visited;
	}

	Game game{};
	GameContext ctx{};
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ PLAYER_COLUMN, CORRIDOR_ROW }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
	Creature goblin{
		Vector2D{ GOBLIN_START_COLUMN, CORRIDOR_ROW },
		ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK } };
};

// The whole report: a monster walks into water it cannot swim and drowns there.
TEST_F(MonsterWaterAvoidanceTest, AMonsterThatCannotSwimNeverStandsInWater)
{
	ASSERT_FALSE(goblin.has_state(ActorState::CAN_SWIM)) << "the goblin under test must not be a swimmer";

	for (const Vector2D& standing : walk_the_goblin())
	{
		EXPECT_FALSE(map.is_water(standing))
			<< "the goblin stood in water at column " << standing.x;
	}
}

// It has to stop at the edge rather than anywhere: a monster that never moves would
// pass the test above while being just as broken.
TEST_F(MonsterWaterAvoidanceTest, AMonsterClosesTheDistanceAndStopsOnTheBank)
{
	walk_the_goblin();

	EXPECT_EQ(goblin.position.x, BANK_COLUMN)
		<< "the goblin should close to the dry tile beside the water and hold there";
	EXPECT_EQ(goblin.position.y, CORRIDOR_ROW);
}

// The bypass still has to work, or the fix is just a wall across the water.
TEST_F(MonsterWaterAvoidanceTest, AMonsterThatCanSwimCrossesTheWater)
{
	goblin.add_state(ActorState::CAN_SWIM);

	walk_the_goblin();

	EXPECT_LT(goblin.position.x, WATER_COLUMN)
		<< "a swimmer should cross the water and reach the player's side";
}

// The other way in, and the likelier one: a monster drifts long before it ever sees the
// player, and drift picks its direction off a die rather than off the gradient. Nothing
// covered this until a mutation sweep reported it as a claim no test made.
TEST_F(MonsterWaterAvoidanceTest, DriftingDoesNotCarryAMonsterIntoWater)
{
	// Beyond the player's sight, so the turn falls to the drift branch instead of
	// hunting, and inside fifteen tiles so that is the branch that runs.
	constexpr int POOL_COLUMN = 14;
	constexpr int DRIFTER_COLUMN = 15;
	map.set_tile(Vector2D{ POOL_COLUMN, CORRIDOR_ROW }, TileType::WATER, 10.0);
	goblin.position = Vector2D{ DRIFTER_COLUMN, CORRIDOR_ROW };
	ASSERT_FALSE(map.is_in_fov(goblin.position)) << "an unseen drifter is the point; a seen one hunts instead";

	// Queued in the order decide_action reads them. The fixture builds its own Game, so
	// the queue starts empty and nothing has to clear it first - measured, the drift
	// lands west on 20 of 20 attempts.
	game.dice.set_next_roll(2); // the d6 misses the hunt
	game.dice.set_next_roll(1); // the d10 takes the drift
	game.dice.set_next_roll(-1); // one step west, straight at the pool
	game.dice.set_next_roll(0);

	goblin.ai->update(goblin, ctx);

	EXPECT_FALSE(map.is_water(goblin.position))
		<< "the goblin drifted into water at (" << goblin.position.x << "," << goblin.position.y << ")";
	EXPECT_EQ(goblin.position.x, DRIFTER_COLUMN) << "the step into the pool was refused, so it stands where it stood";
}

// The same drift carries a swimmer in, so what holds the goblin is the swim rule rather
// than anything about the direction it was given.
TEST_F(MonsterWaterAvoidanceTest, TheSameDriftCarriesASwimmerIn)
{
	constexpr int POOL_COLUMN = 14;
	constexpr int DRIFTER_COLUMN = 15;
	map.set_tile(Vector2D{ POOL_COLUMN, CORRIDOR_ROW }, TileType::WATER, 10.0);
	goblin.position = Vector2D{ DRIFTER_COLUMN, CORRIDOR_ROW };
	goblin.add_state(ActorState::CAN_SWIM);

	game.dice.set_next_roll(2);
	game.dice.set_next_roll(1);
	game.dice.set_next_roll(-1);
	game.dice.set_next_roll(0);

	goblin.ai->update(goblin, ctx);

	EXPECT_EQ(goblin.position.x, POOL_COLUMN) << "a swimmer takes the step the goblin was refused";
	EXPECT_TRUE(map.is_water(goblin.position));
}

namespace
{

// The room the second fixture carves, in map coordinates. One scattered water tile,
// which is all Map::spawn_water ever makes: it flips a 5 percent coin per floor tile
// independently, so a pool wide enough to need a detour is not what the generator
// produces - a single tile beside the player is.
//
//   . . . . . G . . .      row 2, the goblin
//   . . . . . . . . .      row 3
//   . . . . . . ~ . .      row 4, the water
//   . . . . . . . P .      row 5, the player
//   . . . . . . . . .      row 6
//
// The goblin walks to row 3 and then wants the water, because the water is the only
// tile nearer the player than the one it is standing on. Every dry tile beside it is
// the same distance away, not nearer. So the question this fixture asks is whether a
// creature will step sideways to get round a tile it may not enter.
constexpr int ROOM_COL = 2;
constexpr int ROOM_ROW = 2;
constexpr int ROOM_COL_END = 10;
constexpr int ROOM_ROW_END = 6;
constexpr Vector2D POOL{ 8, 4 };
constexpr Vector2D PLAYER_TILE{ 9, 5 };
constexpr Vector2D GOBLIN_TILE{ 7, 2 };

} // namespace

class MonsterWaterDetourTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_dr(0);
		player->set_thaco(20);
		player->set_strength(10);
		player->set_dexterity(10);

		goblin.experienceReward = std::make_unique<ExperienceReward>(0);
		goblin.healthPool = std::make_unique<HealthPool>(STARTING_HP);
		goblin.armorClass = std::make_unique<ArmorClass>(10);
		goblin.attacker = std::make_unique<MonsterAttacker>(goblin, DamageInfo{ "1d4", DamageType::PHYSICAL });
		goblin.set_dr(0);
		goblin.set_thaco(20);
		goblin.set_strength(8);
		goblin.set_dexterity(10);
		goblin.set_natural_attack("bite");
		goblin.ai = std::make_unique<AiMonster>();

		ctx = game.context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		ctx.creatures = &creatures;

		map.init_tiles();
		for (int row = ROOM_ROW; row <= ROOM_ROW_END; ++row)
		{
			for (int column = ROOM_COL; column <= ROOM_COL_END; ++column)
			{
				map.set_tile(Vector2D{ column, row }, TileType::FLOOR, 1.0);
			}
		}
		map.set_tile(POOL, TileType::WATER, 10.0);
		map.compute_fov(ctx);
		map.rebuild_dijkstra_map({ player->position }, ctx);

		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	int distance_to_player() const
	{
		return goblin.get_tile_distance(player->position);
	}

	Game game{};
	GameContext ctx{};
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(PLAYER_TILE) };
	std::vector<std::unique_ptr<Creature>> creatures{};
	Creature goblin{ GOBLIN_TILE, ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK } };
};

// A monster must not be stopped by a puddle it could walk around. Barring water left
// the tiles beside it all the same distance from the player, and a rule that demands a
// strictly nearer tile cannot step across that - so the goblin stands two tiles away
// and never attacks.
TEST_F(MonsterWaterDetourTest, AMonsterWalksRoundAPuddleInsteadOfStoppingAtIt)
{
	ASSERT_FALSE(goblin.has_state(ActorState::CAN_SWIM)) << "the goblin under test must not be a swimmer";
	ASSERT_EQ(distance_to_player(), 3) << "the fixture should start the goblin three tiles out";
	ASSERT_TRUE(map.is_in_fov(goblin.position)) << "an aware goblin hunts; an unseen one drifts on a die instead";

	for (int round = 0; round < ROUNDS; ++round)
	{
		goblin.ai->update(goblin, ctx);
		EXPECT_FALSE(map.is_water(goblin.position))
			<< "round " << round << ": the goblin stood in water at (" << goblin.position.x << "," << goblin.position.y << ")";
	}

	EXPECT_LE(distance_to_player(), 1)
		<< "the goblin held at (" << goblin.position.x << "," << goblin.position.y
		<< "), " << distance_to_player() << " tiles out, because the only nearer tile was the water";
}
