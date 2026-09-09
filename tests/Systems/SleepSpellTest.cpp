// file: SleepSpellTest.cpp
// Checks Sleep against AD&D 2e, Player's Handbook page 279:
//
//   "The spell affects 2d4 Hit Dice of monsters. Monsters with 4+3 Hit Dice
//    (4 Hit Dice plus 3 hit points) or more are unaffected... The creatures
//    with the least Hit Dice are affected first, and partial effects are
//    ignored."  Duration: 5 rounds per level. Undead are excluded.
//
// The implementation rolled 2d8, capped nothing, slept undead, ignored caster
// level, and claimed in a comment to sort by Hit Dice without sorting.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/ActorTypes/Player.h"
#include "src/Combat/ExperienceReward.h"
#include "src/Map/Map.h"
#include "src/Systems/BuffSystem.h"
#include "src/Systems/SpellSystem.h"
#include "tests/mocks/MockGameContext.h"

class SleepSpellTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.creatures = &creatures;
		ctx.playerOwner = &caster;
		ctx.map = &map;
		ctx.buffSystem = &buffs;

		caster->healthPool = std::make_unique<HealthPool>(20);
		caster->armorClass = std::make_unique<ArmorClass>(10);
		caster->experienceReward = std::make_unique<ExperienceReward>(0);
		caster->set_creature_level(3);

		map.init_tiles();

		// An open row so everything placed on it is in view of the caster.
		for (int col = 1; col < 19; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR, 1.0);
		}
		caster->position = Vector2D{ 2, 5 };
		map.compute_fov(ctx);
	}

	Creature& add_creature(int hitDice, int column, bool undead)
	{
		auto creature = std::make_unique<Creature>(
			Vector2D{ column, 5 }, ActorData{ TileRef{}, undead ? "skeleton" : "goblin", 0 });
		creature->healthPool = std::make_unique<HealthPool>(8);
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creature->set_creature_level(hitDice);
		creature->set_undead(undead);
		creatures.push_back(std::move(creature));
		return *creatures.back();
	}

	void force_next_roll(int value)
	{
		mock.dice.set_next_roll(value);
	}

	// The public path the game uses; cast_sleep itself is private.
	void cast_sleep()
	{
		SpellSystem::cast_spell_by_key("sleep", *caster, [](GameContext&) {}, ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
	Map map{ 20, 20 };
	std::unique_ptr<Player> caster{ std::make_unique<Player>(Vector2D{ 2, 5 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// Undead do not sleep, whatever the budget.
TEST_F(SleepSpellTest, UndeadAreNeverAffected)
{
	Creature& skeleton = add_creature(1, 4, true);
	force_next_roll(8); // the largest 2d4 can give

	cast_sleep();

	EXPECT_FALSE(skeleton.has_state(ActorState::IS_SLEEPING));
}

// The book puts anything above four Hit Dice out of reach.
TEST_F(SleepSpellTest, CreaturesAboveFourHitDiceAreUnaffected)
{
	Creature& ogre = add_creature(5, 4, false);
	force_next_roll(8);

	cast_sleep();

	EXPECT_FALSE(ogre.has_state(ActorState::IS_SLEEPING));
}

// A creature at exactly the threshold is still reachable.
TEST_F(SleepSpellTest, FourHitDiceIsStillAffected)
{
	Creature& gnoll = add_creature(4, 4, false);
	force_next_roll(8);

	cast_sleep();

	EXPECT_TRUE(gnoll.has_state(ActorState::IS_SLEEPING));
}

// The budget is spent on the weakest first, whatever order they were added in.
TEST_F(SleepSpellTest, WeakestAreAffectedFirst)
{
	Creature& strong = add_creature(4, 4, false);
	Creature& weak = add_creature(1, 6, false);
	force_next_roll(2); // covers the 1 HD creature and nothing else

	cast_sleep();

	EXPECT_TRUE(weak.has_state(ActorState::IS_SLEEPING)) << "the weakest must be reached first";
	EXPECT_FALSE(strong.has_state(ActorState::IS_SLEEPING)) << "the budget could not cover it";
}

// Partial effects are ignored: a creature the budget cannot cover stays awake.
TEST_F(SleepSpellTest, PartialEffectsAreIgnored)
{
	Creature& gnoll = add_creature(3, 4, false);
	force_next_roll(2); // less than its Hit Dice

	cast_sleep();

	EXPECT_FALSE(gnoll.has_state(ActorState::IS_SLEEPING));
}

// Duration is 5 rounds per caster level, which is the rule the unused caster
// parameter was standing for: the implementation hardcoded 5 turns regardless.
TEST_F(SleepSpellTest, DurationScalesWithCasterLevel)
{
	Creature& goblin = add_creature(1, 4, false);
	force_next_roll(4);

	cast_sleep();

	ASSERT_TRUE(goblin.has_state(ActorState::IS_SLEEPING));
	EXPECT_EQ(buffs.get_buff_turns(goblin, BuffType::SLEEP), 15) << "3rd level caster: 5 rounds per level";
}

// A higher-level caster puts them under for longer.
TEST_F(SleepSpellTest, HigherLevelCasterSleepsThemLonger)
{
	caster->set_creature_level(7);
	Creature& goblin = add_creature(1, 4, false);
	force_next_roll(4);

	cast_sleep();

	ASSERT_TRUE(goblin.has_state(ActorState::IS_SLEEPING));
	EXPECT_EQ(buffs.get_buff_turns(goblin, BuffType::SLEEP), 35);
}
// Every spell effect that lands calls onSuccess, at four sites and sometimes a
// turn later from a targeting callback. An empty one used to throw
// bad_function_call from deep inside that callback; it now fails at the entry
// point, naming the caller's mistake.
TEST_F(SleepSpellTest, CastingWithoutACallbackIsRefused)
{
	add_creature(1, 4, false);
	force_next_roll(4);

	EXPECT_DEATH(SpellSystem::cast_spell_by_key("sleep", *caster, {}, ctx), "requires a callback");
}
