// file: TurnUndeadTest.cpp
// Verifies who may turn undead, which creatures an attempt reaches, and what
// each outcome does. Table 61 itself is checked in TurningTableTest; this covers
// the system built on it.

#include <gtest/gtest.h>

#include <algorithm>

#include "src/Colors.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/Map.h"
#include "src/Player.h"
#include "src/RandomDice.h"
#include "src/TileType.h"
#include "src/TurnUndead.h"

using namespace Turning;
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

class TurnUndeadTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.creatures = &creatures;

		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);

		// A destroyed undead reaches Creature::die, which pays the player its
		// kill reward, so the player needs one too.
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->playerClassState = Player::PlayerClassState::CLERIC;
		player->set_creature_level(7);
	}

	// Places one undead near the player. Hit dice come from creature level,
	// which is the row a turning attempt reads.
	Creature& add_undead(int hitDice, Vector2D position)
	{
		auto undead = std::make_unique<Creature>(position, ActorData{ TileRef{}, "skeleton", ColorPairId::WHITE_BLACK });
		undead->healthPool = std::make_unique<HealthPool>(8);

		// Every creature the game builds has one; die() reads it for the kill reward.
		undead->experienceReward = std::make_unique<ExperienceReward>(0);
		undead->set_undead(true);
		undead->set_creature_level(hitDice);
		creatures.push_back(std::move(undead));
		return *creatures.back();
	}

	Creature& add_living(Vector2D position)
	{
		auto living = std::make_unique<Creature>(position, ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK });
		living->healthPool = std::make_unique<HealthPool>(8);
		creatures.push_back(std::move(living));
		return *creatures.back();
	}

	// The attempt reads a d20, then two d6 for how many undead are affected. Each die
	// is named, because a sum of two dice is not one roll: scripting the cap as a single
	// value is how a cap of 1, which 2d6 cannot produce, got into two tests here.
	void script_attempt(int d20Roll, int firstDie, int secondDie)
	{
		mock.dice.set_next_roll(d20Roll);
		mock.dice.set_next_roll(firstDie);
		mock.dice.set_next_roll(secondDie);
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 5, 5 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// Only a cleric channels a deity.
TEST_F(TurnUndeadTest, NonClericCannotAttempt)
{
	player->playerClassState = Player::PlayerClassState::FIGHTER;
	add_undead(1, Vector2D{ 6, 5 });

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_FALSE(report.attempted);
	EXPECT_TRUE(report.turned.empty());
	EXPECT_TRUE(report.destroyed.empty());
}

// A 7th-level priest destroys a 1 HD skeleton outright: Table 61 gives "D".
TEST_F(TurnUndeadTest, StrongPriestDestroysWeakUndead)
{
	Creature& skeleton = add_undead(1, Vector2D{ 6, 5 });
	script_attempt(10, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_TRUE(report.attempted);
	ASSERT_EQ(report.destroyed.size(), 1u);
	EXPECT_EQ(report.destroyed.front(), &skeleton);
	EXPECT_TRUE(skeleton.is_dead());
}

// A wight needs a 4 at level 7; the roll clears it, so the wight flees.
TEST_F(TurnUndeadTest, SuccessfulRollSetsTheUndeadFleeing)
{
	Creature& wight = add_undead(5, Vector2D{ 6, 5 });
	script_attempt(12, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.turned.size(), 1u);
	EXPECT_EQ(report.turned.front(), &wight);
	EXPECT_TRUE(wight.has_state(ActorState::IS_FLEEING));
	EXPECT_FALSE(wight.is_dead()) << "turning drives off, it does not kill";
}

// A spectre needs a 16 at level 7; a 12 falls short and it is unmoved.
TEST_F(TurnUndeadTest, FailedRollLeavesTheUndeadUnmoved)
{
	Creature& spectre = add_undead(8, Vector2D{ 6, 5 });
	script_attempt(12, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.resisted.size(), 1u);
	EXPECT_EQ(report.resisted.front(), &spectre);
	EXPECT_FALSE(spectre.has_state(ActorState::IS_FLEEING));
}

// Living creatures are not undead and are never touched.
TEST_F(TurnUndeadTest, LivingCreaturesAreIgnored)
{
	Creature& goblin = add_living(Vector2D{ 6, 5 });
	script_attempt(20, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_TRUE(report.turned.empty());
	EXPECT_TRUE(report.destroyed.empty());
	EXPECT_TRUE(report.resisted.empty());
	EXPECT_FALSE(goblin.has_state(ActorState::IS_FLEEING));
}

// An undead beyond the priest's reach is not affected.
TEST_F(TurnUndeadTest, UndeadOutOfRangeIsUnaffected)
{
	Creature& distant = add_undead(1, Vector2D{ 5 + TURN_UNDEAD_RANGE + 1, 5 });
	script_attempt(20, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_TRUE(report.destroyed.empty());
	EXPECT_FALSE(distant.is_dead());
}

// The book rolls once for the whole attempt and reads it per type, so a single
// roll can turn one undead and fail against another.
TEST_F(TurnUndeadTest, OneRollIsReadAgainstEveryType)
{
	Creature& wight = add_undead(5, Vector2D{ 6, 5 }); // needs 4
	Creature& spectre = add_undead(8, Vector2D{ 4, 5 }); // needs 16
	script_attempt(12, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_EQ(report.roll, 12);
	ASSERT_EQ(report.turned.size(), 1u);
	EXPECT_EQ(report.turned.front(), &wight);
	ASSERT_EQ(report.resisted.size(), 1u);
	EXPECT_EQ(report.resisted.front(), &spectre);
}

// The book caps a successful turn at 2d6 undead however many are present.
TEST_F(TurnUndeadTest, TheTwoDSixCapLimitsHowManyAreAffected)
{
	constexpr int skeletons = 4;
	for (int placed = 0; placed < skeletons; ++placed)
	{
		add_undead(1, Vector2D{ 6, 4 + placed });
	}
	script_attempt(10, 1, 1); // the d20, then 2d6 of 2

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_EQ(report.destroyed.size(), 2u) << "the 2d6 cap did not bind";
	EXPECT_EQ(report.resisted.size(), 2u) << "the undead past the cap were not left alone";
}

// "If the undead are a mixed group, the lowest Hit Dice creatures are turned
// first." The wight is placed first, so only the sort can put the skeletons ahead
// of it. Three undead and a cap of two, because the lowest 2d6 can roll is two and a
// cap cannot bind on a group no larger than itself.
TEST_F(TurnUndeadTest, TheWeakestAreAffectedFirstWhenTheCapBinds)
{
	Creature& wight = add_undead(5, Vector2D{ 6, 5 });
	Creature& firstSkeleton = add_undead(1, Vector2D{ 4, 5 });
	Creature& secondSkeleton = add_undead(1, Vector2D{ 4, 6 });
	script_attempt(20, 1, 1); // the d20 clears every target number, then 2d6 of two

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.destroyed.size(), 2u);
	EXPECT_EQ(report.destroyed.at(0), &firstSkeleton) << "the tougher undead was taken first";
	EXPECT_EQ(report.destroyed.at(1), &secondSkeleton) << "the tougher undead was taken first";
	ASSERT_EQ(report.resisted.size(), 1u);
	EXPECT_EQ(report.resisted.front(), &wight);
}

// The far edge of the priest's reach is inside it. Without this the range check
// is only ever tested from beyond, where an off-by-one reads as correct.
TEST_F(TurnUndeadTest, UndeadAtTheEdgeOfRangeIsAffected)
{
	Creature& skeleton = add_undead(1, Vector2D{ 5 + TURN_UNDEAD_RANGE, 5 });
	script_attempt(20, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.destroyed.size(), 1u);
	EXPECT_EQ(report.destroyed.front(), &skeleton) << "the edge of the range was treated as outside it";
}

// A body on the floor is not turned again, and does not spend one of the 2d6. Two
// standing undead against a cap of two: if the corpse took a slot, only one of them
// would fall. The lowest 2d6 can roll is two, so the cap cannot be made smaller.
TEST_F(TurnUndeadTest, AlreadyDeadUndeadAreSkipped)
{
	Creature& corpse = add_undead(1, Vector2D{ 6, 5 });
	corpse.set_hp(0);
	Creature& firstStanding = add_undead(1, Vector2D{ 4, 5 });
	Creature& secondStanding = add_undead(1, Vector2D{ 4, 6 });
	script_attempt(20, 1, 1); // the d20, then 2d6 of two

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.destroyed.size(), 2u) << "a corpse was counted against the cap";
	EXPECT_EQ(report.destroyed.at(0), &firstStanding);
	EXPECT_EQ(report.destroyed.at(1), &secondStanding);
}

// "If the number rolled is equal to or greater than that listed, the attempt is
// successful." A wight needs a 4 at level 7, and a 4 is enough.
TEST_F(TurnUndeadTest, ARollEqualToTheNumberNeededSucceeds)
{
	Creature& wight = add_undead(5, Vector2D{ 6, 5 });
	script_attempt(4, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.turned.size(), 1u) << "meeting the number was read as falling short";
	EXPECT_EQ(report.turned.front(), &wight);
}

// The reach is a square, not a circle. get_tile_distance is Chebyshev, so an undead
// at the far corner is as near as one straight ahead - a test that only ever places
// undead on an axis cannot tell the two apart.
TEST_F(TurnUndeadTest, AnUndeadOnTheDiagonalIsReachedAtTheSameRange)
{
	Creature& skeleton = add_undead(1, Vector2D{ 5 + TURN_UNDEAD_RANGE, 5 + TURN_UNDEAD_RANGE });
	script_attempt(20, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.destroyed.size(), 1u) << "the reach was measured as a circle rather than a square";
	EXPECT_EQ(report.destroyed.front(), &skeleton);
}

// Four tiles, stated as a number. Every other range test here places its undead
// relative to TURN_UNDEAD_RANGE and so moves with it, which means none of them can
// see the constant itself change - the same blind spot a mutation sweep found in
// PlayerRestTest on 2026-10-01.
TEST_F(TurnUndeadTest, AnUndeadFiveTilesAwayIsOutOfReach)
{
	add_undead(1, Vector2D{ 10, 5 });
	script_attempt(20, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_TRUE(report.destroyed.empty()) << "an undead five tiles away was reached";
	EXPECT_TRUE(report.turned.empty());
}

// The deviation, pinned. The book gives turning no reach at all and bounds only the
// aftermath, so this game's radius is its own rule - and that radius is distance
// alone, with nothing consulting the map. This test exists so that adding line of
// sight is a decision somebody makes rather than a drift nobody notices: it fails
// the moment a wall starts blocking a turn.
TEST_F(TurnUndeadTest, AWallBetweenPriestAndUndeadDoesNotStopTheTurning)
{
	Map map{ 20, 20 };
	// The constructor leaves the tile grid empty; init_tiles fills it before any
	// set_tile, or the subscript is out of range.
	map.init_tiles();
	for (int col = 1; col < 19; ++col)
	{
		map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR, 1.0);
	}
	map.set_tile(Vector2D{ 6, 5 }, TileType::WALL, 0.0);
	ctx.map = &map;

	Creature& skeleton = add_undead(1, Vector2D{ 7, 5 });
	script_attempt(20, 3, 3); // the d20, then 2d6 of 6

	const TurnUndeadReport report = turn_undead(*player, ctx);

	ASSERT_EQ(report.destroyed.size(), 1u) << "a wall stopped a turn, which is a rule change rather than a repair";
	EXPECT_EQ(report.destroyed.front(), &skeleton);
}

// "2d6 undead are affected" - two six-sided dice, so the cap runs from 2 to 12 and
// sits on 7 more often than on either end. The call read TURN_UNDEAD_DICE_COUNT as a
// minimum and TURN_UNDEAD_DICE_SIDES as a maximum, which is a flat 2 to 6: a priest
// could never affect a seventh undead, and every count between 2 and 6 was equally
// likely. Twelve skeletons stand here so the cap is the only thing that can bind.
TEST_F(TurnUndeadTest, TheCapReachesTwelveBecauseItIsTwoDice)
{
	// Three columns beside the priest at {5,5}, every tile of them inside
	// TURN_UNDEAD_RANGE, so the reach cannot be what limits the count.
	constexpr int skeletons = 12;
	for (int placed = 0; placed < skeletons; ++placed)
	{
		add_undead(1, Vector2D{ 6 + placed / 4, 4 + placed % 4 });
	}
	script_attempt(20, 6, 6); // the d20, then 2d6 of twelve

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_EQ(report.destroyed.size(), 12u) << "two sixes affected fewer than twelve undead";
}

// The other end of the same rule: two ones are two, never one, so a cap of one is a
// number the dice cannot produce.
TEST_F(TurnUndeadTest, TheCapStartsAtTwoBecauseItIsTwoDice)
{
	constexpr int skeletons = 5;
	for (int placed = 0; placed < skeletons; ++placed)
	{
		add_undead(1, Vector2D{ 6, 4 + placed });
	}
	script_attempt(20, 1, 1); // the d20, then 2d6 of two

	const TurnUndeadReport report = turn_undead(*player, ctx);

	EXPECT_EQ(report.destroyed.size(), 2u) << "two ones did not affect exactly two undead";
}

// The dice the book prints, asserted in absolute terms. Every case above scripts the
// dice by value, and a scripted value is returned whatever range was asked for
// (issues/0072), so no test that scripts can see the die's size change underneath it.
// "2d6 undead are affected" is the whole source for both numbers.
TEST(TurnUndeadDiceTest, TheCapIsTwoSixSidedDice)
{
	EXPECT_EQ(TURN_UNDEAD_DICE_COUNT, 2) << "the book affects 2d6 undead, not another count";
	EXPECT_EQ(TURN_UNDEAD_DICE_SIDES, 6) << "the book affects 2d6 undead, not another die";
}

// What the two dice can actually produce, drawn rather than scripted, because the
// range a die is asked for is the one thing a scripted roll cannot show. Two summed
// d6 reach both 2 and 12; the defect this replaced passed the count where the minimum
// belongs, which gives a flat 2 to 6 - so a 12 never appears. Rolling the count and
// the size as a range instead puts each die at 2 to 6, where a 2 never appears.
//
// 2,000 draws: the chance of missing an end of a 2d6 by luck is (35/36)^2000, which is
// 3.4e-25. That is the whole of the statistics here - an observed extreme either
// occurs or the range cannot produce it.
TEST(TurnUndeadDiceTest, TwoDiceReachBothEndsOfTheirRange)
{
	RandomDice dice{};
	int lowest = TURN_UNDEAD_DICE_COUNT * TURN_UNDEAD_DICE_SIDES;
	int highest = 0;

	for (int draw = 0; draw < 2000; ++draw)
	{
		const int cap = roll_turning_cap(dice);
		lowest = std::min(lowest, cap);
		highest = std::max(highest, cap);
	}

	EXPECT_EQ(lowest, 2) << "the lowest two dice can show is two";
	EXPECT_EQ(highest, 12) << "the highest two dice can show is twelve";
}
