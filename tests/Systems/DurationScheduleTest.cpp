// file: DurationScheduleTest.cpp
// How long a thing lasts when its owner is fast.
//
// Under the time-unit turn order a creature's update runs once per action rather than
// once per round, so anything counted down inside it is counted down as often as its
// owner acts. A duration measured that way is not a duration at all: it is a count of
// its owner's turns, and Haste would halve the duration of Haste.
//
// The claim every case here makes is the same one, stated against the alternative that
// pace is what changes: a duration printed in rounds lasts those rounds whoever it is
// on. The quickling and the ordinary creature are driven through the same windows and
// their timers run out together.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=DurationScheduleTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/Ai.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Creature.h"
#include "src/CreatureManager.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/TurnSchedule.h"
#include "tests/mocks/CountingAi.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// The blessing every buff case here uses, in rounds, as a priest's Bless prints it.
constexpr int BLESS_ROUNDS = 3;

// How long the confusion case confuses for, in rounds.
constexpr int CONFUSION_ROUNDS = 3;
} // namespace

class DurationScheduleTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.creatures = &creatures;
		ctx.creatureManager = &manager;
	}

	// A creature with the three parts Creature::update reaches through, and a mind
	// that counts rather than one that needs a map to walk on.
	Creature& placed(int actionDelay)
	{
		auto creature = std::make_unique<Creature>(
			Vector2D{ 5, 5 },
			ActorData{ TileRef{}, "subject", ColorPairId::WHITE_BLACK });
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creature->armorClass = std::make_unique<ArmorClass>(10);
		creature->healthPool = std::make_unique<HealthPool>(10);
		creature->set_action_delay(actionDelay);
		creature->ai = std::make_unique<CountingAi>();

		creatures.push_back(std::move(creature));
		return *creatures.back();
	}

	// Queue rolls that read as standing still, so a confused creature needs no map.
	void script_still_directions(int count)
	{
		for (int roll = 0; roll < count; ++roll)
		{
			mock.dice.set_next_roll(0);
		}
	}

	// Drive every creature through the window that ends one round further on.
	void run_round(int round)
	{
		ctx.gameState->set_time(TIME_UNITS_PER_ROUND * round);
		manager.update_creatures(creatures, ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
	CreatureManager manager{};
	BuffSystem buffs{};
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// The defect, stated as the property it breaks. Two creatures are blessed for the same
// three rounds and driven through the same windows; the quickling acts twice in each.
// Its blessing has to outlast the second round exactly as the ordinary creature's does.
TEST_F(DurationScheduleTest, SpeedDoesNotChangeHowLongABuffLasts)
{
	Creature& ordinary = placed(TIME_UNITS_PER_ROUND);
	Creature& quickling = placed(TIME_UNITS_PER_ROUND / 2);

	buffs.add_buff(ordinary, BuffType::BLESS, 0, BLESS_ROUNDS, false, ctx.gameState->get_time());
	buffs.add_buff(quickling, BuffType::BLESS, 0, BLESS_ROUNDS, false, ctx.gameState->get_time());

	for (int round = 1; round < BLESS_ROUNDS; ++round)
	{
		run_round(round);

		EXPECT_TRUE(buffs.has_buff(ordinary, BuffType::BLESS)) << "round " << round;
		EXPECT_TRUE(buffs.has_buff(quickling, BuffType::BLESS))
			<< "the quickling's blessing ran out in round " << round << " of " << BLESS_ROUNDS;
	}

	run_round(BLESS_ROUNDS);

	EXPECT_FALSE(buffs.has_buff(ordinary, BuffType::BLESS));
	EXPECT_FALSE(buffs.has_buff(quickling, BuffType::BLESS)) << "the quickling's blessing outstayed its rounds";
}

// The quickling really did act twice in each window, which is what makes the case above
// a measurement rather than two creatures being treated identically by accident.
TEST_F(DurationScheduleTest, TheQuicklingUnderTestActsTwicePerRound)
{
	Creature& ordinary = placed(TIME_UNITS_PER_ROUND);
	Creature& quickling = placed(TIME_UNITS_PER_ROUND / 2);

	run_round(1);

	EXPECT_EQ(static_cast<CountingAi*>(ordinary.ai.get())->updates, 1);
	EXPECT_EQ(static_cast<CountingAi*>(quickling.ai.get())->updates, 2);
}

// A buff granted part-way through a game runs from the reading it was granted at. An
// expiry worked out from the start of the game instead would hand a creature blessed on
// round nine a blessing that ended six rounds before it was cast.
TEST_F(DurationScheduleTest, ABuffRunsFromTheReadingItWasGrantedAt)
{
	Creature& subject = placed(TIME_UNITS_PER_ROUND);

	const int granted = 9;
	ctx.gameState->set_time(TIME_UNITS_PER_ROUND * granted);
	buffs.add_buff(subject, BuffType::BLESS, 0, BLESS_ROUNDS, false, ctx.gameState->get_time());

	for (int round = granted + 1; round < granted + BLESS_ROUNDS; ++round)
	{
		run_round(round);
		EXPECT_TRUE(buffs.has_buff(subject, BuffType::BLESS)) << "round " << round;
	}

	run_round(granted + BLESS_ROUNDS);
	EXPECT_FALSE(buffs.has_buff(subject, BuffType::BLESS));
}

// AD&D 2e stacking: a second casting of the same spell takes the better value and the
// later ending. A weaker one cast over a blessing with longer to run must not cut it
// short - the creature already has the better deal and keeps it.
TEST_F(DurationScheduleTest, AWeakerSecondCastingCanOnlyLengthenTheOneRunning)
{
	Creature& subject = placed(TIME_UNITS_PER_ROUND);

	buffs.add_buff(subject, BuffType::SHIELD, 4, 10, false, 0);
	buffs.add_buff(subject, BuffType::SHIELD, 1, 2, false, 0);

	EXPECT_EQ(buffs.get_buff_turns(subject, BuffType::SHIELD, 0), 10) << "the shorter casting cut the longer one short";
	EXPECT_EQ(buffs.get_buff_value(subject, BuffType::SHIELD), 4) << "the weaker casting lowered the bonus";
}

// The count a screen would print never reads zero while the buff is in force, which is
// the invariant that lets rounds left be shown at all. A quickling is the creature that
// produces part-rounds, so it is the one asked.
TEST_F(DurationScheduleTest, ABuffStillInForceNeverReportsNoRoundsLeft)
{
	Creature& quickling = placed(TIME_UNITS_PER_ROUND / 2);

	buffs.add_buff(quickling, BuffType::BLESS, 0, BLESS_ROUNDS, false, 0);

	for (int halfRound = 1; halfRound < BLESS_ROUNDS * 2; ++halfRound)
	{
		const int currentTime = (TIME_UNITS_PER_ROUND / 2) * halfRound;
		ctx.gameState->set_time(currentTime);
		manager.update_creatures(creatures, ctx);

		ASSERT_TRUE(buffs.has_buff(quickling, BuffType::BLESS)) << "gone at " << currentTime;
		EXPECT_GT(buffs.get_buff_turns(quickling, BuffType::BLESS, currentTime), 0) << "no rounds left at " << currentTime;
	}
}

// The same property for a spell that replaces the creature's mind rather than adding to
// its buffs. A confused creature's own update is what ends the confusion, so a creature
// acting twice in a round used to shake it off in half the rounds the scroll named.
TEST_F(DurationScheduleTest, SpeedDoesNotChangeHowLongConfusionLasts)
{
	Creature& ordinary = placed(TIME_UNITS_PER_ROUND);
	Creature& quickling = placed(TIME_UNITS_PER_ROUND / 2);

	ordinary.apply_confusion(CONFUSION_ROUNDS, 0);
	quickling.apply_confusion(CONFUSION_ROUNDS, 0);

	// A confused creature rolls two dice for a direction and walks it. Zero on both is
	// the one direction that stays put, so it never asks the map anything - and the map
	// is the whole reason a confused creature would otherwise need a dungeon to be in.
	// Nine updates over the three rounds, two rolls each, and a margin: if the queue runs
	// dry the dice turn real and the walk reaches a map that is not there.
	script_still_directions(40);

	for (int round = 1; round < CONFUSION_ROUNDS; ++round)
	{
		run_round(round);

		EXPECT_EQ(ordinary.ai->get_ai_type(), AiType::CONFUSED_MONSTER) << "round " << round;
		EXPECT_EQ(quickling.ai->get_ai_type(), AiType::CONFUSED_MONSTER)
			<< "the quickling's head cleared in round " << round << " of " << CONFUSION_ROUNDS;
	}

	run_round(CONFUSION_ROUNDS);

	EXPECT_EQ(ordinary.ai->get_ai_type(), AiType::MONSTER) << "the confusion outstayed its rounds";
	EXPECT_EQ(quickling.ai->get_ai_type(), AiType::MONSTER) << "the confusion outstayed its rounds";
}

// end of file: DurationScheduleTest.cpp
