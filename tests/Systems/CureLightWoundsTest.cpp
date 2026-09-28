// file: CureLightWoundsTest.cpp
// Checks Cure Light Wounds against AD&D 2e, Player's Handbook PDF page 428:
//
//   "When casting this spell and laying his hand upon a creature, the priest
//    causes 1d8 points of wound or other injury damage to the creature's body
//    to be healed."
//
// So the whole of the spell's number is 1d8. Two things follow that the roll
// alone does not say: a body cannot hold more than its maximum, so a cast on an
// almost-whole creature is mostly wasted; and the line the player reads has to
// report what landed rather than what was rolled, or it claims healing that
// never happened.
//
// The die's size cannot be checked with a scripted roll - in TESTING_MODE
// RandomDice returns the queued value whatever the range, so a d6 would pass a
// test that queues an 8. The range test below seeds a real generator instead
// and casts until both ends of the die have shown.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=CureLightWoundsTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/MessageSystem.h"
#include "src/Player.h"
#include "src/RandomDice.h"
#include "src/SpellSystem.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int MAX_HP = 20;
// Table 5 gives a caster this wise no chance of failure, so the fizzle gate
// short-circuits and consumes none of the rolls queued below.
constexpr int WISDOM_THAT_NEVER_FIZZLES = 18;
// Enough casts that a d8 shows both its ends; the seed makes it the same run
// every time.
constexpr int CASTS_FOR_THE_RANGE = 200;
constexpr unsigned int RANGE_SEED = 20260928u;
} // namespace

class CureLightWoundsTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &caster;

		caster->healthPool = std::make_unique<HealthPool>(MAX_HP);
		caster->armorClass = std::make_unique<ArmorClass>(10);
		caster->experienceReward = std::make_unique<ExperienceReward>(0);
		caster->set_creature_level(1);
		caster->set_wisdom(WISDOM_THAT_NEVER_FIZZLES);

		mock.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		mock.dice.set_test_mode(false);
		mock.dice.clear_fixed_rolls();
	}

	// The public path the game uses; cast_cure_light_wounds itself is private.
	void cast()
	{
		SpellSystem::cast_spell_by_key("cure_light_wounds", *caster, SpellSource::MEMORIZED, [](GameContext&) {}, ctx);
	}

	// Every part of every finalized message, joined. What the spell healed is
	// stated only here.
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
	std::unique_ptr<Player> caster{ std::make_unique<Player>(Vector2D{ 2, 5 }) };
};

// "causes 1d8 points of... damage to the creature's body to be healed" - with
// room to take all of it, the roll is what the body gets.
TEST_F(CureLightWoundsTest, TheSpellHealsWhatTheDieRolled)
{
	caster->set_hp(10);
	mock.dice.set_next_roll(5);

	cast();

	EXPECT_EQ(caster->get_hp(), 15);
}

// A body holds no more than its maximum, so a cast on an almost-whole creature
// spends the rest of the roll on nothing.
TEST_F(CureLightWoundsTest, HealingStopsAtTheMaximum)
{
	caster->set_hp(MAX_HP - 2);
	mock.dice.set_next_roll(8);

	cast();

	EXPECT_EQ(caster->get_hp(), MAX_HP);
}

// The line the player reads is the only account of what the spell did, and a
// report of a change is a function of the health before and after - not of the
// die. Two of the eight rolled here were real.
TEST_F(CureLightWoundsTest, TheMessageReportsWhatLandedRatherThanWhatWasRolled)
{
	caster->set_hp(MAX_HP - 2);
	mock.dice.set_next_roll(8);

	cast();

	EXPECT_NE(all_message_text().find("+2 HP"), std::string::npos) << "the message claims more than the body took";
	EXPECT_EQ(all_message_text().find("+8 HP"), std::string::npos);
}

// The die is a d8. Scripted rolls cannot say so - they are returned whatever the
// range asked for - so this seeds a real generator and reads the range back off
// the healing itself. A d6 never reaches 8; a d10 passes 8.
TEST_F(CureLightWoundsTest, TheSpellRollsAnEightSidedDie)
{
	mock.dice.set_test_mode(false);
	RandomDice seeded{ RANGE_SEED };
	ctx.dice = &seeded;

	int lowest = MAX_HP;
	int highest = 0;
	for (int castNumber = 0; castNumber < CASTS_FOR_THE_RANGE; ++castNumber)
	{
		// One hit point, so the whole of any roll this die can make still fits.
		caster->set_hp(1);
		cast();

		const int healed = caster->get_hp() - 1;
		lowest = std::min(lowest, healed);
		highest = std::max(highest, healed);
	}

	EXPECT_EQ(lowest, 1) << "the die's low end";
	EXPECT_EQ(highest, 8) << "the die's high end";
}

// end of file: CureLightWoundsTest.cpp
