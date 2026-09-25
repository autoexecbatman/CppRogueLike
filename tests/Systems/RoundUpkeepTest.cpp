// file: RoundUpkeepTest.cpp
// One round of upkeep reaches everyone standing in it: the player and every creature
// regenerate, feel their poison, grow hungrier, and the clock moves.
//
// What it is for. The per-round block lived in the middle of GameLoopCoordinator::update,
// a frame handler that also drives menus, the GUI and the state machine, so nothing could
// call it without wiring a whole game. Every piece of it was tested alone and none of it
// was tested as the loop runs it - deleting either regeneration call from the loop left
// the whole suite green. The two regenerations are separated here on purpose: one
// creature heals only through Constitution and the other only through a ring, so removing
// either call fails exactly one case.
//
// The round number is what both rules read, and both land on round 10: Constitution 25
// regenerates a point every turn, a ring of regeneration heals a point every turn, and a
// turn is ten rounds. So the upkeep is run ten times and the tenth is the one that heals.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=RoundUpkeepTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/CurseSystem.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/GameLoopCoordinator.h"
#include "src/HealthPool.h"
#include "src/HungerSystem.h"
#include "src/Item.h"
#include "src/Player.h"
#include "tests/mocks/MockGameContext.h"

namespace
{

constexpr int STARTING_HP = 20;
constexpr int ROUNDS_IN_A_TURN = 10;

} // namespace

class RoundUpkeepTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		mock.dice.set_test_mode(false);

		player = std::make_unique<Player>(Vector2D{ 1, 1 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->set_constitution(10);
		// A Player built here keeps creatureClass at MONSTER, and the regeneration rule
		// reads that field rather than playerClassState.
		player->set_creature_class(CreatureClass::FIGHTER);

		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.creatures = &creatures;
		ctx.hungerSystem = &hungerSystem;
		ctx.curseSystem = &curseSystem;
		ctx.gameLoopCoordinator = &coordinator;
	}

	// Given a class, because regenerate_from_constitution returns early for MONSTER.
	// That early return is the game's, not the book's: the DMG's monster ability scores
	// say every Constitution modifier applies to a creature as to a character.
	Creature& add_creature(const std::string& name, int constitution)
	{
		auto creature = std::make_unique<Creature>(Vector2D{ 5, 5 }, ActorData{ TileRef{}, name, 1 });
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creature->armorClass = std::make_unique<ArmorClass>(10);
		creature->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		creature->set_creature_class(CreatureClass::FIGHTER);
		creature->set_constitution(constitution);
		creatures.push_back(std::move(creature));
		return *creatures.back();
	}

	void run_rounds(int count)
	{
		for (int round = 0; round < count; ++round)
		{
			coordinator.apply_round_upkeep(ctx);
		}
	}

	MockGameContext mock{};
	GameContext ctx{};
	GameLoopCoordinator coordinator{};
	HungerSystem hungerSystem{};
	CurseSystem curseSystem{};
	std::unique_ptr<Player> player{};
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// Constitution 25 mends a point a turn, and the upkeep is what asks it to.
TEST_F(RoundUpkeepTest, AConstitutionThatMendsHealsOnTheTurnsRound)
{
	Creature& mender = add_creature("mender", 25);
	mender.healthPool->set_hp(STARTING_HP - 5);

	// The first upkeep also lands the Constitution bonus on maximum and current hit
	// points, so the baseline is taken after that rather than before it.
	run_rounds(1);
	const int settled = mender.healthPool->get_hp();

	run_rounds(ROUNDS_IN_A_TURN - 2);
	EXPECT_EQ(mender.healthPool->get_hp(), settled) << "healed before the turn was up";

	run_rounds(1);
	EXPECT_EQ(mender.healthPool->get_hp(), settled + 1)
		<< "the upkeep never asked Constitution to regenerate";
}

// And a ring mends a point a turn for someone Constitution does nothing for, so the two
// calls cannot cover for each other.
TEST_F(RoundUpkeepTest, ARingOfRegenerationHealsOnTheTurnsRound)
{
	Creature& wearer = add_creature("wearer", 10);
	wearer.healthPool->set_hp(STARTING_HP - 5);
	const int wounded = wearer.healthPool->get_hp();

	// A hand to wear it on: a creature built in a test has no body until it is given one.
	wearer.set_body_plan({ EquipmentSlot::RIGHT_RING });
	auto ring = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "ring of regeneration", 1 });
	ring->behavior = MagicalRing{ MagicalEffect::REGENERATION, 0 };
	wearer.wear(std::move(ring), EquipmentSlot::RIGHT_RING);
	ASSERT_TRUE(wearer.wears_ring_of(MagicalEffect::REGENERATION)) << "the ring is not on";

	run_rounds(ROUNDS_IN_A_TURN - 1);
	EXPECT_EQ(wearer.healthPool->get_hp(), wounded) << "healed before the turn was up";

	run_rounds(1);
	EXPECT_EQ(wearer.healthPool->get_hp(), wounded + 1)
		<< "the upkeep never asked the ring to regenerate";
}

// The player is not in the creature list, so the upkeep has to reach them separately.
TEST_F(RoundUpkeepTest, ThePlayerFeelsTheRoundToo)
{
	player->set_constitution(25);
	player->healthPool->set_hp(STARTING_HP - 5);

	run_rounds(1);
	const int settled = player->healthPool->get_hp();

	run_rounds(ROUNDS_IN_A_TURN - 1);

	EXPECT_EQ(player->healthPool->get_hp(), settled + 1)
		<< "the upkeep reached the creatures and skipped the player";
}

// A dose that is due lands during the upkeep, for a creature as well as for the player.
TEST_F(RoundUpkeepTest, PoisonComesDueDuringTheUpkeep)
{
	Creature& bitten = add_creature("bitten", 10);
	const int before = bitten.healthPool->get_hp();
	bitten.take_poison(1, 3);

	run_rounds(1);

	EXPECT_EQ(bitten.healthPool->get_hp(), before - 3) << "the dose never came due";
}

// Hunger is the other thing a round costs, and the clock is what everything else reads.
TEST_F(RoundUpkeepTest, HungerRisesAndTheClockMoves)
{
	const int hungerBefore = hungerSystem.get_hunger_value();
	const int timeBefore = ctx.gameState->get_time();

	run_rounds(3);

	EXPECT_GT(hungerSystem.get_hunger_value(), hungerBefore) << "three rounds cost no hunger";
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore + 3) << "the clock did not move";
}
