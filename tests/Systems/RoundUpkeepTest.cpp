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
#include <string>
#include <vector>

#include "src/Colors.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/GameContext.h"
#include "src/GameLoopCoordinator.h"
#include "src/TurnSchedule.h"
#include "src/HealthPool.h"
#include "src/HungerSystem.h"
#include "src/Item.h"
#include "src/ItemClassification.h"
#include "src/ItemIdentification.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
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
		// reads that field.
		player->set_creature_class(CreatureClass::FIGHTER);

		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.creatures = &creatures;
		ctx.hungerSystem = &hungerSystem;
		ctx.gameLoopCoordinator = &coordinator;

		// A round reaches every body on the level, and a body has to be standing
		// somewhere: holding a breath asks the map whether that somewhere is water.
		ctx.map = &map;
		map.init_tiles();
	}

	// Given a class, because regenerate_from_constitution returns early for MONSTER.
	// That early return is the game's, not the book's: the DMG's monster ability scores
	// say every Constitution modifier applies to a creature as to a character.
	Creature& add_creature(const std::string& name, int constitution)
	{
		auto creature = std::make_unique<Creature>(Vector2D{ 5, 5 }, ActorData{ TileRef{}, name, ColorPairId::WHITE_BLACK });
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creature->armorClass = std::make_unique<ArmorClass>(10);
		creature->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		creature->set_creature_class(CreatureClass::FIGHTER);
		creature->set_constitution(constitution);
		creatures.push_back(std::move(creature));
		return *creatures.back();
	}

	// A round of upkeep follows a round of time passing. The clock is what decides
	// whether a round is owed, so a fixture that called the upkeep without letting
	// time pass would ask it to feed a round that had not happened.
	void run_rounds(int count)
	{
		for (int round = 0; round < count; ++round)
		{
			ctx.gameState->advance_clock(TIME_UNITS_PER_ROUND);
			coordinator.apply_round_upkeep(ctx);
		}
	}

	// One action that takes this many rounds: the clock moves by all of them and the
	// upkeep runs once, as it does after any action the player takes.
	void spend_one_action_of(int rounds)
	{
		ctx.gameState->advance_clock(TIME_UNITS_PER_ROUND * rounds);
		coordinator.apply_round_upkeep(ctx);
	}

	std::string all_message_text() const
	{
		std::string joined;
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			for (const auto& part : ctx.messageSystem->get_attack_message_at(index))
			{
				joined += part.text;
			}
		}
		return joined;
	}

	MockGameContext mock{};
	GameContext ctx{};
	GameLoopCoordinator coordinator{};
	Map map{ 20, 20 };
	HungerSystem hungerSystem{};
	std::unique_ptr<Player> player{};
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// A cursed item is found by what it does and by refusing to come off, as the book's cursed
// items are: a round worn in a cursed sword and a cursed amulet announces nothing and costs
// no hit points.
TEST_F(RoundUpkeepTest, CursedGearSaysNothingAndTakesNothingEachRound)
{
	player->set_body_plan(mock.body_plans.get("humanoid"));
	auto sword = std::make_unique<Item>(Vector2D{ 1, 1 }, ActorData{ TileRef{}, "sword", ColorPairId::WHITE_BLACK });
	sword->itemClass = ItemClass::SWORD;
	sword->enhancement.blessing = BlessingStatus::CURSED;
	auto amulet = std::make_unique<Item>(Vector2D{ 1, 1 }, ActorData{ TileRef{}, "amulet", ColorPairId::WHITE_BLACK });
	amulet->itemClass = ItemClass::AMULET;
	amulet->enhancement.blessing = BlessingStatus::CURSED;
	player->wear(std::move(sword), EquipmentSlot::RIGHT_HAND);
	player->wear(std::move(amulet), EquipmentSlot::NECK);
	const int hitPointsBefore = player->healthPool->get_hp();
	const size_t messagesBefore = mock.messages.get_stored_message_count();

	run_rounds(1);

	EXPECT_EQ(player->healthPool->get_hp(), hitPointsBefore) << "a cursed item cost hit points for a round of wearing it";
	// Hunger speaks in its first round; nothing may speak of the curse.
	for (size_t index = messagesBefore; index < mock.messages.get_stored_message_count(); ++index)
	{
		std::string whole;
		for (const auto& run : mock.messages.get_attack_message_at(index))
		{
			whole += run.text;
		}
		EXPECT_EQ(whole.find("curse"), std::string::npos) << "a round of wearing cursed gear was announced: " << whole;
		EXPECT_EQ(whole.find("aim"), std::string::npos) << whole;
	}
}

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
	auto ring = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "ring of regeneration", ColorPairId::WHITE_BLACK });
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
	bitten.take_poison(PendingPoison{ .roundsUntilOnset = 1, .damage = 3 });

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

	// The clock counts time units, so three rounds is three rounds' worth of them.
	EXPECT_EQ(ctx.gameState->get_time(), timeBefore + TIME_UNITS_PER_ROUND * 3) << "the clock did not move";

	// And three rounds of upkeep were fed, which is the fact the hunger above rests
	// on: a round that passed without being fed would leave the clock right and the
	// body untouched.
	EXPECT_EQ(ctx.gameState->get_rounds_run(), 3) << "a round passed without being fed";
}

// An action that takes seven rounds is seven rounds of upkeep: every round that finished
// is fed, not only the first.
TEST_F(RoundUpkeepTest, ASevenRoundActionPaysSevenRoundsOfHunger)
{
	const int hungerBefore = hungerSystem.get_hunger_value();

	spend_one_action_of(7);

	EXPECT_EQ(ctx.gameState->get_rounds_run(), 7) << "rounds finished without being fed";
	EXPECT_EQ(hungerSystem.get_hunger_value(), hungerBefore + 7);
}

// And a one-round action is one round, so paying the backlog cannot overpay.
TEST_F(RoundUpkeepTest, AOneRoundActionPaysExactlyOneRound)
{
	const int hungerBefore = hungerSystem.get_hunger_value();

	spend_one_action_of(1);
	coordinator.apply_round_upkeep(ctx);

	EXPECT_EQ(ctx.gameState->get_rounds_run(), 1);
	EXPECT_EQ(hungerSystem.get_hunger_value(), hungerBefore + 1) << "a round was paid twice";
}

// A dose with seven rounds to go lands inside a seven-round action; one with eight does
// not, and lands on the round after.
TEST_F(RoundUpkeepTest, APoisonDueWithinALongActionLandsInIt)
{
	Creature& due = add_creature("due", 10);
	Creature& later = add_creature("later", 10);
	const int before = due.healthPool->get_hp();
	due.take_poison(PendingPoison{ .roundsUntilOnset = 7, .damage = 3 });
	later.take_poison(PendingPoison{ .roundsUntilOnset = 8, .damage = 3 });

	spend_one_action_of(7);
	EXPECT_EQ(due.healthPool->get_hp(), before - 3) << "the dose due on round 7 never landed";
	EXPECT_EQ(later.healthPool->get_hp(), before) << "the dose due on round 8 landed early";

	spend_one_action_of(1);
	EXPECT_EQ(later.healthPool->get_hp(), before - 3) << "the dose due on round 8 never landed";
}

// Regeneration is due on round 10, and an action covering rounds 2 to 10 heals once:
// each round is fed under its own number.
TEST_F(RoundUpkeepTest, RegenerationLandsOnceOnItsRoundInsideALongAction)
{
	Creature& mender = add_creature("mender", 25);
	mender.healthPool->set_hp(STARTING_HP - 5);
	run_rounds(1);
	const int settled = mender.healthPool->get_hp();

	spend_one_action_of(ROUNDS_IN_A_TURN - 1);

	EXPECT_EQ(mender.healthPool->get_hp(), settled + 1) << "round 10 healed other than once";
}

// A creature that dies on round 3 of 7 is gone before round 4. Its air runs out at the
// action's end, so each round it is fed is one breath check: the first three pass, and
// a fourth would drown it again - a second death and a second reward.
TEST_F(RoundUpkeepTest, ACreatureThatDiesMidActionIsNotFedTheRoundsAfter)
{
	map.set_tile(Vector2D{ 5, 5 }, TileType::WATER);
	Creature& diver = add_creature("diver", 10);
	diver.set_air_until_time(7 * TIME_UNITS_PER_ROUND);
	diver.take_poison(PendingPoison{ .roundsUntilOnset = 3, .damage = STARTING_HP });
	for (const int roll : { 1, 1, 1, 20, 20, 20, 20 })
	{
		mock.dice.set_next_roll(roll);
	}

	spend_one_action_of(7);

	EXPECT_TRUE(creatures.empty()) << "the dead diver is still on the level";
	EXPECT_EQ(player->get_kill_count(), 1) << "the diver died more than once";
	EXPECT_EQ(all_message_text().find("drowns"), std::string::npos) << all_message_text();
}

// The player is never erased, so the rounds stop when the player dies.
TEST_F(RoundUpkeepTest, ThePlayerWhoDiesMidActionIsNotFedTheRoundsAfter)
{
	map.set_tile(Vector2D{ 1, 1 }, TileType::WATER);
	player->set_air_until_time(7 * TIME_UNITS_PER_ROUND);
	player->take_poison(PendingPoison{ .roundsUntilOnset = 3, .damage = STARTING_HP });
	for (const int roll : { 1, 1, 1, 20, 20, 20, 20 })
	{
		mock.dice.set_next_roll(roll);
	}

	spend_one_action_of(7);

	EXPECT_TRUE(player->is_dead());
	EXPECT_EQ(ctx.gameState->get_rounds_run(), 3) << "rounds were fed to a dead player";
	EXPECT_EQ(all_message_text().find("drowns"), std::string::npos) << all_message_text();
}

// A venom that debilitates starts its condition when its dose lands, in the round it
// lands in. Landing on round 3 of a seven-round action, ten rounds of it run to the end
// of round 13, so at the action's end six remain.
TEST_F(RoundUpkeepTest, AVenomLandingInsideALongActionStartsInItsOwnRound)
{
	Creature& bitten = add_creature("bitten", 10);
	bitten.take_poison(PendingPoison{
		.roundsUntilOnset = 3,
		.damage = 0,
		.effect = BuffType::HAIRY_SPIDER_VENOM,
		.effectValue = 1,
		.effectRounds = 10 });

	spend_one_action_of(7);

	EXPECT_EQ(ctx.buffSystem->get_buff_turns(bitten, BuffType::HAIRY_SPIDER_VENOM, ctx.gameState->get_time()), 6)
		<< "the venom started at the action's end, not the round it landed in";
}

// end of file: RoundUpkeepTest.cpp
