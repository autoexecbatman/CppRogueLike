// file: SpellFailureTest.cpp
// A priest's spells fizzle when its Wisdom is low, by Table 5's Chance of Spell
// Failure column (Player's Handbook, PDF page 37).
//
// The book's whole rule: "Chance of Spell Failure states the percentage chance that
// any particular spell fails when cast. Priests with low Wisdom scores run the risk
// of having their spells fizzle. Roll percentile dice every time the priest casts a
// spell; if the number rolled is less than or equal to the listed chance for spell
// failure, the spell is expended with absolutely no effect whatsoever. Note that
// priests with Wisdom scores of 13 or higher don't need to worry about their spells
// failing."
//
// Two claims carry the weight. The roll is against the caster's own row, so 9 is a
// fifth of castings and 13 is none. And a fizzle costs the spell: "expended with
// absolutely no effect" is not the same as nothing happening, which is what a
// success-only callback would have made it.
//
// Every roll is scripted. A cleric needs Wisdom 9 to exist at all, so 9 to 12 is the
// whole live range.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SpellFailureTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/DataManager.h"
#include "src/HealthPool.h"
#include "src/MessageSystem.h"
#include "src/SpellSystem.h"
#include "tests/mocks/MockGameContext.h"

class SpellFailureTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.dataManager = &dataManager;
		dataManager.load_all_data(messages);

		priest.healthPool = std::make_unique<HealthPool>(20);
		priest.armorClass = std::make_unique<ArmorClass>(10);
		priest.set_creature_class(CreatureClass::CLERIC);
		priest.set_wisdom(9);
	}

	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	MessageSystem messages{};
	Creature priest{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "priest", ColorPairId::WHITE_BLACK } };
};

// Table 5's column, read straight down the rows a priest can have.
TEST_F(SpellFailureTest, TheChanceIsTableFivesColumn)
{
	priest.set_wisdom(9);
	EXPECT_EQ(SpellSystem::spell_failure_chance(priest, dataManager), 20);
	priest.set_wisdom(10);
	EXPECT_EQ(SpellSystem::spell_failure_chance(priest, dataManager), 15);
	priest.set_wisdom(11);
	EXPECT_EQ(SpellSystem::spell_failure_chance(priest, dataManager), 10);
	priest.set_wisdom(12);
	EXPECT_EQ(SpellSystem::spell_failure_chance(priest, dataManager), 5);
}

// "priests with Wisdom scores of 13 or higher don't need to worry".
TEST_F(SpellFailureTest, WisdomThirteenAndAboveNeverFizzles)
{
	for (const int wisdom : { 13, 14, 16, 18 })
	{
		priest.set_wisdom(wisdom);
		EXPECT_EQ(SpellSystem::spell_failure_chance(priest, dataManager), 0) << "Wisdom " << wisdom;
	}
}

// The column is Wisdom's, and Wisdom is the priest's prime requisite. A wizard casts
// on Intelligence and the book gives it no failure chance at all.
TEST_F(SpellFailureTest, OnlyAPriestFizzles)
{
	priest.set_creature_class(CreatureClass::WIZARD);
	priest.set_wisdom(9);

	EXPECT_EQ(SpellSystem::spell_failure_chance(priest, dataManager), 0);
}

// "if the number rolled is less than or equal to the listed chance" - so at a chance
// of 20 a roll of 20 fizzles and a roll of 21 does not.
TEST_F(SpellFailureTest, TheRollIsAtOrUnderTheChance)
{
	priest.set_wisdom(9);

	mock.dice.set_next_roll(20);
	EXPECT_TRUE(SpellSystem::spell_fizzles(priest, dataManager, *ctx.dice));

	mock.dice.set_next_roll(21);
	EXPECT_FALSE(SpellSystem::spell_fizzles(priest, dataManager, *ctx.dice));
}

// A priest who cannot fizzle is not made to roll: an unscripted roll would be a real
// one, so the queued 1 still being there is what says the dice were never asked.
TEST_F(SpellFailureTest, ACasterWhoCannotFizzleDoesNotRoll)
{
	priest.set_wisdom(13);

	mock.dice.set_next_roll(1);
	EXPECT_FALSE(SpellSystem::spell_fizzles(priest, dataManager, *ctx.dice));

	// The 1 is still queued, so this Wisdom 9 priest spends it and fizzles.
	priest.set_wisdom(9);
	EXPECT_TRUE(SpellSystem::spell_fizzles(priest, dataManager, *ctx.dice));
}

// "the spell is expended with absolutely no effect whatsoever": the casting is over
// and the spell is gone, which is a different thing from the casting not happening.
TEST_F(SpellFailureTest, AFizzledSpellIsSpentAndDoesNothing)
{
	priest.set_wisdom(9);
	priest.set_hp(10);

	bool spent = false;
	const auto onCastComplete = [&spent](GameContext&)
	{
		spent = true;
	};

	// 20 is at the chance, so it fizzles; cure light wounds would otherwise heal 1d8
	// and its die is left unscripted, so a spell that landed would roll a real one.
	mock.dice.set_next_roll(20);
	SpellSystem::cast_spell_by_key("cure_light_wounds", priest, SpellSource::MEMORIZED, onCastComplete, ctx);

	EXPECT_TRUE(spent) << "the spell is expended even though it did nothing";
	EXPECT_EQ(priest.get_hp(), 10) << "and it healed nothing";
}

// The same priest, one point of Wisdom higher on the roll, is healed.
TEST_F(SpellFailureTest, ASpellThatDoesNotFizzleLands)
{
	priest.set_wisdom(9);
	priest.set_hp(10);

	bool spent = false;
	const auto onCastComplete = [&spent](GameContext&)
	{
		spent = true;
	};

	// 21 is past the chance, then the healing die.
	mock.dice.set_next_roll(21);
	mock.dice.set_next_roll(5);
	SpellSystem::cast_spell_by_key("cure_light_wounds", priest, SpellSource::MEMORIZED, onCastComplete, ctx);

	EXPECT_TRUE(spent);
	EXPECT_EQ(priest.get_hp(), 15) << "1d8 showing five";
}

// A spell a ring or a helm casts is not the priest's own, so its Wisdom does not
// decide whether it works.
TEST_F(SpellFailureTest, AnItemsSpellDoesNotFizzle)
{
	priest.set_wisdom(9);
	priest.set_hp(10);

	bool spent = false;
	const auto onCastComplete = [&spent](GameContext&)
	{
		spent = true;
	};

	// A 1 would fizzle any memorised casting; here it is the healing die instead,
	// because the item's spell never asks whether the priest's Wisdom failed it.
	mock.dice.set_next_roll(1);
	SpellSystem::cast_spell_by_key("cure_light_wounds", priest, SpellSource::ITEM, onCastComplete, ctx);

	EXPECT_TRUE(spent);
	EXPECT_EQ(priest.get_hp(), 11) << "1d8 showing one";
}

// end of file: SpellFailureTest.cpp
