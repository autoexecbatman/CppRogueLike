// file: DrowningTest.cpp
// How long a lungful lasts, and how fast the odds fall after it.
//
// The numbers are the Player's Handbook's, PDF pages 239 and 240 of the archive, and
// every expectation here is worked out from that text rather than from a run:
//
//   "a character can hold his breath up to 1/3 his Constitution score in rounds
//    (rounded up)... All characters are able to hold their breath for one round,
//    regardless of circumstances."
//
//   "The first check has no modifiers, but each subsequent check suffers a -2
//    cumulative penalty."
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DrowningTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/CurseSystem.h"
#include "src/Drowning.h"
#include "src/ExperienceReward.h"
#include "src/GameLoopCoordinator.h"
#include "src/HealthPool.h"
#include "src/HungerSystem.h"
#include "src/Item.h"
#include "src/MagicalItemEffects.h"
#include "src/Map.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "tests/mocks/MockGameContext.h"

// A third of the score, and the book says rounded up rather than down.
TEST(DrowningTest, BreathIsAThirdOfConstitutionRoundedUp)
{
	EXPECT_EQ(breath_rounds(18), 6);
	EXPECT_EQ(breath_rounds(15), 5);
	EXPECT_EQ(breath_rounds(14), 5) << "4.67 rounds up to 5, it does not truncate to 4";
	EXPECT_EQ(breath_rounds(13), 5) << "4.33 rounds up to 5";
	EXPECT_EQ(breath_rounds(12), 4);
}

// "All characters are able to hold their breath for one round, regardless of
// circumstances" - so the floor is the rule and not a guard against division.
TEST(DrowningTest, EveryoneGetsAtLeastOneRound)
{
	EXPECT_EQ(breath_rounds(3), 1) << "a third of 3 is 1 anyway";
	EXPECT_EQ(breath_rounds(2), 1) << "a third of 2 rounds up to 1";
	EXPECT_EQ(breath_rounds(1), 1);
	EXPECT_EQ(breath_rounds(0), 1) << "the floor holds even where the table does not reach";
}

// The first check past the allowance is unmodified, and each one after is two worse.
TEST(DrowningTest, TheFirstCheckIsUnmodifiedAndEachOneAfterIsTwoWorse)
{
	EXPECT_EQ(breath_check_penalty(0), 0);
	EXPECT_EQ(breath_check_penalty(1), -2);
	EXPECT_EQ(breath_check_penalty(2), -4);
	EXPECT_EQ(breath_check_penalty(5), -10);
}

// It is cumulative rather than a flat penalty, which is the difference a single point
// cannot show: the gap between consecutive rounds is always two.
TEST(DrowningTest, ThePenaltyAccumulatesByTwoEveryRound)
{
	for (int round = 1; round <= 10; ++round)
	{
		EXPECT_EQ(breath_check_penalty(round) - breath_check_penalty(round - 1), -2)
			<< "between round " << round - 1 << " and " << round;
	}
}

// A creature that has not yet run out is not being penalised at all, which is what
// keeps the clock and the check from disagreeing at the boundary.
TEST(DrowningTest, NothingBeforeTheAllowanceRunsOut)
{
	EXPECT_EQ(breath_check_penalty(-1), 0);
	EXPECT_EQ(breath_check_penalty(-4), 0);
}

// The rules above, applied to a creature standing in water for real.
class DrowningInWaterTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		player = std::make_unique<Player>(Vector2D{ 1, 1 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(30);

		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.creatures = &creatures;
		ctx.map = &map;
		ctx.hungerSystem = &hungerSystem;
		ctx.curseSystem = &curseSystem;
		ctx.gameLoopCoordinator = &coordinator;

		map.init_tiles();
		map.set_tile(Vector2D{ 5, 5 }, TileType::WATER, 1.0);
	}

	// A swimmer standing in the one water tile, able to be there and still breathing air.
	Creature& swimmer_in_water(int constitution)
	{
		auto creature = std::make_unique<Creature>(Vector2D{ 5, 5 }, ActorData{ TileRef{}, "diver", ColorPairId::WHITE_BLACK });
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creature->armorClass = std::make_unique<ArmorClass>(10);
		creature->healthPool = std::make_unique<HealthPool>(30);
		creature->set_constitution(constitution);
		creatures.push_back(std::move(creature));
		return *creatures.back();
	}

	void round_passes()
	{
		ctx.gameState->advance_clock(TIME_UNITS_PER_ROUND);
		coordinator.apply_round_upkeep(ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 20 };
	GameLoopCoordinator coordinator{};
	HungerSystem hungerSystem{};
	CurseSystem curseSystem{};
	std::unique_ptr<Player> player{};
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// The round a creature is found in water it takes a breath, and the clock knows when
// that breath runs out.
TEST_F(DrowningInWaterTest, BeingInWaterStartsTheBreath)
{
	Creature& diver = swimmer_in_water(15);

	ASSERT_FALSE(diver.get_air_until_time().has_value()) << "nothing is held before a round runs";

	round_passes();

	ASSERT_TRUE(diver.get_air_until_time().has_value()) << "a creature in water is holding its breath";
	EXPECT_EQ(*diver.get_air_until_time(), TIME_UNITS_PER_ROUND * (1 + breath_rounds(15)))
		<< "five rounds of air from the round it was found in";
}

// Surfacing ends it. The breath is not a countdown something has to cancel - standing
// anywhere dry on any round is enough, so no move has to notice it was the last one.
TEST_F(DrowningInWaterTest, SurfacingClearsTheBreath)
{
	Creature& diver = swimmer_in_water(15);
	round_passes();
	ASSERT_TRUE(diver.get_air_until_time().has_value());

	diver.position = Vector2D{ 2, 2 };
	round_passes();

	EXPECT_FALSE(diver.get_air_until_time().has_value()) << "it surfaced and is still holding its breath";
}

// Past the allowance the checks begin, and a failed one drowns. Constitution 3 buys the
// floor of one round, and a d20 of 20 fails a check against 3 however it is modified.
TEST_F(DrowningInWaterTest, AFailedCheckDrowns)
{
	Creature& diver = swimmer_in_water(3);

	round_passes(); // the gulp of air, and Constitution 3 buys only the floor of one round
	EXPECT_FALSE(diver.is_dead()) << "the first round underwater is the breath, not a check";
	ASSERT_TRUE(diver.get_air_until_time().has_value());
	EXPECT_EQ(*diver.get_air_until_time(), TIME_UNITS_PER_ROUND * 2);

	mock.dice.set_next_roll(20);
	round_passes(); // the allowance is spent, so this round checks, and the check fails

	// Nothing of the diver is touched after that round. The round's own upkeep ends with
	// cleanup_dead_creatures, so a creature that drowns is off the level before the call
	// returns and the reference above is dangling - the level is what to ask now.
	EXPECT_TRUE(creatures.empty()) << "the drowned diver is still on the level";
}

// A check that is made costs nothing. The same creature, the same round, a roll it
// passes - which is what keeps the case above from passing for the wrong reason.
TEST_F(DrowningInWaterTest, APassedCheckCostsNothing)
{
	Creature& diver = swimmer_in_water(18);

	round_passes();
	mock.dice.set_next_roll(1);
	round_passes();

	EXPECT_FALSE(diver.is_dead()) << "a 1 against a Constitution of 18 is a made check";
}

// The helm, which is the whole point. "The possessor is able to see and breathe
// underwater" - Dungeon Master Guide, PDF page 970 - so there is no breath to run out
// and no check to fail, however long it stays under.
TEST_F(DrowningInWaterTest, TheHelmOfUnderwaterActionMeansNoBreathAtAll)
{
	Creature& diver = swimmer_in_water(3);
	auto helm = std::make_unique<Item>(Vector2D{}, ActorData{ TileRef{}, "helm of underwater action", ColorPairId::CYAN_BLACK });
	helm->itemKey = "helm_of_underwater_action";
	helm->behavior = MagicalHelm{ MagicalEffect::UNDERWATER_ACTION, 0 };
	diver.equippedItems.emplace_back(std::move(helm), EquipmentSlot::HEAD);

	ASSERT_FALSE(diver.needs_air_underwater()) << "the helm keeps a globe of air about the head";

	for (int round = 0; round < 10; ++round)
	{
		round_passes();
	}

	EXPECT_FALSE(diver.get_air_until_time().has_value()) << "it never started holding a breath";
	EXPECT_FALSE(diver.is_dead()) << "ten rounds under and the helm still held";
}

// A creature that lives in water breathes it. The gauntlets of swimming carry a visitor
// across and say nothing about air, which is the difference the helm is sold on.
TEST_F(DrowningInWaterTest, SomethingThatLivesInWaterNeverHoldsItsBreath)
{
	Creature& fish = swimmer_in_water(3);
	fish.add_state(ActorState::CAN_SWIM);

	for (int round = 0; round < 10; ++round)
	{
		round_passes();
	}

	EXPECT_FALSE(fish.get_air_until_time().has_value());
	EXPECT_FALSE(fish.is_dead()) << "a water dweller drowned in its own element";
}

// A flying creature over water is not in it. The Monstrous Manual gives a bat
// "Movement: 1, Fl 24 (B)" and a wyvern "6, Fl 24 (E)", so what crosses water on wings
// is not holding a breath and has nothing to fail a check against.
TEST_F(DrowningInWaterTest, AFlyingCreatureOverWaterHoldsNoBreath)
{
	Creature& bat = swimmer_in_water(3);
	bat.add_state(ActorState::CAN_FLY);

	for (int round = 0; round < 10; ++round)
	{
		round_passes();
	}

	EXPECT_FALSE(bat.get_air_until_time().has_value()) << "a flier took a breath it had no need of";
	EXPECT_FALSE(bat.is_dead()) << "a bat drowned flying over a puddle";
}

// Flight and swimming are separate abilities, so neither stands in for the other: a
// creature that only swims is still in the water and still holding its breath.
TEST_F(DrowningInWaterTest, FlyingIsNotSwimming)
{
	Creature& flier = swimmer_in_water(12);
	flier.add_state(ActorState::CAN_FLY);

	EXPECT_FALSE(flier.has_state(ActorState::CAN_SWIM)) << "flight was granted as swimming";
}

// The data has to say so, or the capability is one nothing in the game ever carries.
// The Monstrous Manual gives a bat "Movement: 1, Fl 24 (B)" and a goblin "Movement: 6".
TEST_F(DrowningInWaterTest, ABatBuiltFromTheDataFliesAndAGoblinDoesNot)
{
	const auto bat = MonsterCreator::create_from_params(
		Vector2D{ 5, 5 }, ctx.monsterRegistry->get_params("bat"), ctx);
	const auto goblin = MonsterCreator::create_from_params(
		Vector2D{ 6, 6 }, ctx.monsterRegistry->get_params("goblin"), ctx);

	ASSERT_NE(bat, nullptr);
	ASSERT_NE(goblin, nullptr);
	EXPECT_TRUE(bat->is_flying()) << "the bat does not fly, so it drowns in a puddle";
	EXPECT_FALSE(goblin->is_flying()) << "a goblin was given wings";
}

// A breath already half spent is still half spent after a save. The reading is on the
// shared clock, which the save carries, so what survives is the moment it runs out
// rather than a number of rounds that would start again.
TEST_F(DrowningInWaterTest, ABreathAlreadyHeldSurvivesASave)
{
	Creature& diver = swimmer_in_water(15);
	round_passes();
	const auto heldUntil = diver.get_air_until_time();
	ASSERT_TRUE(heldUntil.has_value());

	json saved;
	diver.save(saved);

	Creature restored{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "diver", ColorPairId::WHITE_BLACK } };
	restored.load(saved);

	EXPECT_EQ(restored.get_air_until_time(), heldUntil) << "the diver surfaced and took a fresh breath across the save";
}

// A creature on dry land writes no breath at all, so the common case costs the save
// nothing and a load cannot invent one.
TEST_F(DrowningInWaterTest, ACreatureOnDryLandSavesNoBreath)
{
	Creature& walker = swimmer_in_water(15);
	walker.position = Vector2D{ 2, 2 };
	round_passes();

	json saved;
	walker.save(saved);

	EXPECT_FALSE(saved.contains("airUntilTime"));
}

// A check is made on or under the score, so a roll exactly equal to the Constitution
// is a pass. Neither of the cases above sits on that line - one rolls 20 against 3 and
// the other 1 against 18 - and a sweep that moved the comparison by one survived both.
TEST_F(DrowningInWaterTest, ARollEqualToTheConstitutionIsAMadeCheck)
{
	Creature& diver = swimmer_in_water(10);

	round_passes(); // four rounds of air from a Constitution of 10
	for (int held = 0; held < breath_rounds(10) - 1; ++held)
	{
		round_passes();
	}

	mock.dice.set_next_roll(10);
	round_passes(); // the first check, unmodified, against exactly 10

	EXPECT_FALSE(creatures.empty()) << "a 10 against a Constitution of 10 drowned the diver";
}

// The penalty accumulates, so the same roll that passes the first check fails a later
// one. Nothing above holds out past the first check, where the penalty is zero either
// way, so a sweep that froze the count survived every case.
TEST_F(DrowningInWaterTest, TheOddsWorsenUntilEvenTheBestRollFails)
{
	swimmer_in_water(3); // the floor: one round of air

	round_passes(); // the gulp

	// A 1 is the best a d20 offers. It passes against 3, passes against 3 - 2, and
	// fails against 3 - 4, so the diver lasts exactly two checks however well it rolls.
	mock.dice.set_next_roll(1);
	round_passes();
	ASSERT_FALSE(creatures.empty()) << "the first check is unmodified and a 1 makes it";

	mock.dice.set_next_roll(1);
	round_passes();
	ASSERT_FALSE(creatures.empty()) << "the second is at -2 and a 1 still makes it";

	mock.dice.set_next_roll(1);
	round_passes();

	EXPECT_TRUE(creatures.empty()) << "the third is at -4, which no roll can make";
}

// The helm on its own is enough to get into the water, as the gauntlets are. Without
// this the helm only mattered to someone who already owned the gauntlets, which is not
// what an item costing 6,000 gold should be worth.
TEST_F(DrowningInWaterTest, TheHelmAloneCarriesAWearerIntoWater)
{
	Creature& diver = swimmer_in_water(10);
	ASSERT_FALSE(diver.has_bypass(ActorState::CAN_SWIM)) << "nothing worn yet";

	auto helm = std::make_unique<Item>(Vector2D{}, ActorData{ TileRef{}, "helm of underwater action", ColorPairId::CYAN_BLACK });
	helm->itemKey = "helm_of_underwater_action";
	helm->behavior = MagicalHelm{ MagicalEffect::UNDERWATER_ACTION, 0 };
	diver.equippedItems.emplace_back(std::move(helm), EquipmentSlot::HEAD);

	EXPECT_TRUE(diver.has_bypass(ActorState::CAN_SWIM)) << "the helm did not carry its wearer into the water";
}

// end of file: DrowningTest.cpp
