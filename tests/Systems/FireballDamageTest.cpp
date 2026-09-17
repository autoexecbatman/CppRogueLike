// file: FireballDamageTest.cpp
// Pins Fireball's damage against AD&D 2e, Player's Handbook page 176:
//
//   "1d6 points of damage per level of the caster (maximum 10d6)... creatures
//    that successfully save versus spell take half damage."
//
// and the game's own resistance rule, DamageResolver: a resistance buff to the
// damage's type removes that percentage of it. Fireball is fire, so a creature
// with 50 percent fire resistance takes half. Every number below is derived from
// those two rules, never from what the code returned.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/ActorTypes/Player.h"
#include "src/Combat/ExperienceReward.h"
#include "src/Systems/BuffSystem.h"
#include "src/Systems/BuffType.h"
#include "src/Systems/SpellSystem.h"
#include "tests/mocks/MockGameContext.h"

class FireballDamageTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.creatures = &creatures;
		ctx.playerOwner = &caster;
		ctx.buffSystem = &buffs;

		caster->healthPool = std::make_unique<HealthPool>(20);
		caster->armorClass = std::make_unique<ArmorClass>(10);
		caster->experienceReward = std::make_unique<ExperienceReward>(0);
		caster->set_creature_level(CASTER_LEVEL);
	}

	// A creature standing at the given column on the caster's row, with more
	// hit points than any burst here can deal, so the damage is readable.
	Creature& add_creature(int column)
	{
		auto creature = std::make_unique<Creature>(
			Vector2D{ column, 5 }, ActorData{ TileRef{}, "goblin", 0 });
		creature->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		creature->armorClass = std::make_unique<ArmorClass>(10);
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creatures.push_back(std::move(creature));
		return *creatures.back();
	}

	// Three sixes for a level-3 caster, then one save roll per creature struck.
	void force_max_dice_then_saves(std::initializer_list<int> saves)
	{
		for (int die = 0; die < CASTER_LEVEL; ++die)
		{
			mock.dice.set_next_roll(6);
		}
		for (const int save : saves)
		{
			mock.dice.set_next_roll(save);
		}
	}

	SpellSystem::FireballBurst burst_at(Vector2D center)
	{
		return SpellSystem::burst_fireball(center, CASTER_LEVEL, RADIUS, ctx);
	}

	static constexpr int CASTER_LEVEL = 3;
	static constexpr int RADIUS = 2;
	static constexpr int STARTING_HP = 30;
	static constexpr int MAX_DAMAGE = CASTER_LEVEL * 6;

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
	std::unique_ptr<Player> caster{ std::make_unique<Player>(Vector2D{ 2, 5 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// Fireball is fire: half of it is resisted by 50 percent fire resistance.
TEST_F(FireballDamageTest, FireResistanceHalvesTheBurn)
{
	Creature& resistant = add_creature(3);
	Creature& exposed = add_creature(4);
	buffs.add_buff(resistant, BuffType::FIRE_RESISTANCE, 50, 10, false);
	force_max_dice_then_saves({ 1, 1 });

	const auto burst = burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(burst.totalDamage, MAX_DAMAGE);
	EXPECT_EQ(burst.struck, 2);
	EXPECT_EQ(exposed.get_hp(), STARTING_HP - MAX_DAMAGE);
	EXPECT_EQ(resistant.get_hp(), STARTING_HP - MAX_DAMAGE / 2)
		<< "fire resistance did nothing, so the burst was not typed as fire";
}

// A save of 15 or better halves the damage before any resistance.
TEST_F(FireballDamageTest, SaveVersusSpellsHalves)
{
	Creature& saved = add_creature(3);
	Creature& failed = add_creature(4);
	force_max_dice_then_saves({ 15, 14 });

	burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(saved.get_hp(), STARTING_HP - MAX_DAMAGE / 2);
	EXPECT_EQ(failed.get_hp(), STARTING_HP - MAX_DAMAGE);
}

// Anything past the radius is untouched, and does not roll a save.
TEST_F(FireballDamageTest, BeyondTheRadiusIsUntouched)
{
	Creature& inside = add_creature(5);
	Creature& outside = add_creature(6);
	force_max_dice_then_saves({ 1 });

	const auto burst = burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(burst.struck, 1);
	EXPECT_EQ(inside.get_hp(), STARTING_HP - MAX_DAMAGE);
	EXPECT_EQ(outside.get_hp(), STARTING_HP);
}

// Ten dice is the ceiling however high the caster's level.
TEST_F(FireballDamageTest, TenDiceIsTheCeiling)
{
	for (int die = 0; die < 10; ++die)
	{
		mock.dice.set_next_roll(1);
	}

	const auto burst = SpellSystem::burst_fireball(Vector2D{ 3, 5 }, 20, RADIUS, ctx);

	EXPECT_EQ(burst.diceCount, 10);
	EXPECT_EQ(burst.totalDamage, 10);
}
