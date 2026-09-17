// file: FireballDamageTest.cpp
// Pins Fireball's damage against AD&D 2e, Player's Handbook page 176:
//
//   "1d6 points of damage per level of the caster (maximum 10d6)... creatures
//    that successfully save versus spell take half damage."
//
// and the Dungeon Master's Guide's fire resistance: a ring's wearer saves at +4
// and takes -2 on every die, never below 1; the helm of brilliance is a double
// ring. The save itself is Table 60: these creatures are 1st-level monsters,
// which save on the warrior row and need 17 against a spell. Every number below
// is derived from those rules, never from the code.

#include <gtest/gtest.h>

#include "src/Actor/Creature.h"
#include "src/ActorTypes/Player.h"
#include "src/Actor/EquipmentSlot.h"
#include "src/Combat/ExperienceReward.h"
#include "src/Core/Paths.h"
#include "src/Factories/ItemCreator.h"
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
		ItemCreator::load(Paths::ITEMS);

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
		creature->set_strength(10);
		creature->set_body_plan({ EquipmentSlot::RIGHT_RING, EquipmentSlot::HEAD });
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

// Fireball is fire: a drunk potion of fire resistance is a ring's worth, -2 a die.
TEST_F(FireballDamageTest, APotionOfFireResistanceTakesTwoOffEveryDie)
{
	Creature& resistant = add_creature(3);
	Creature& exposed = add_creature(4);
	buffs.add_buff(resistant, BuffType::FIRE_RESISTANCE, 1, 10, false);
	force_max_dice_then_saves({ 1, 1 });

	const auto burst = burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(burst.totalDamage, MAX_DAMAGE);
	EXPECT_EQ(burst.struck, 2);
	EXPECT_EQ(exposed.get_hp(), STARTING_HP - MAX_DAMAGE);
	EXPECT_EQ(resistant.get_hp(), STARTING_HP - CASTER_LEVEL * 4)
		<< "fire resistance did nothing, so the burst was not typed as fire";
}

// A made save halves the damage: a 1st-level monster needs 17 against a spell,
// so 17 saves and 16 does not.
TEST_F(FireballDamageTest, SaveVersusSpellsHalves)
{
	Creature& saved = add_creature(3);
	Creature& failed = add_creature(4);
	force_max_dice_then_saves({ 17, 16 });

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

// The book's fire resistance, Dungeon Master's Guide, Ring of Fire Resistance:
// magical fire is "saved against with a +4 bonus to the die roll, and all
// damage dice are calculated at -2 per die, but each die is never less than 1".
// The helm of brilliance is "a double-strength fire resistance ring": +8, -4.
// Three sixes with a ring are three fours; with the helm, three twos.

// A ring's wearer takes -2 on every die of the burst.
TEST_F(FireballDamageTest, ARingOfFireResistanceTakesTwoOffEveryDie)
{
	Creature& wearer = add_creature(3);
	wearer.wear(ItemCreator::create("ring_of_fire_resistance", Vector2D{ 3, 5 }, mock.content_registry), EquipmentSlot::RIGHT_RING);
	force_max_dice_then_saves({ 1 });

	burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(wearer.get_hp(), STARTING_HP - CASTER_LEVEL * 4)
		<< "the ring did not take two off each die";
}

// The helm is double strength: -4 on every die.
TEST_F(FireballDamageTest, TheHelmOfBrillianceTakesFourOffEveryDie)
{
	Creature& wearer = add_creature(3);
	wearer.wear(ItemCreator::create("helm_of_brilliance", Vector2D{ 3, 5 }, mock.content_registry), EquipmentSlot::HEAD);
	force_max_dice_then_saves({ 1 });

	burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(wearer.get_hp(), STARTING_HP - CASTER_LEVEL * 2);
}

// A die is never less than 1: three ones with a ring are still three.
TEST_F(FireballDamageTest, ADieNeverGoesBelowOne)
{
	Creature& wearer = add_creature(3);
	wearer.wear(ItemCreator::create("ring_of_fire_resistance", Vector2D{ 3, 5 }, mock.content_registry), EquipmentSlot::RIGHT_RING);
	for (int die = 0; die < CASTER_LEVEL; ++die)
	{
		mock.dice.set_next_roll(1);
	}
	mock.dice.set_next_roll(1);

	burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(wearer.get_hp(), STARTING_HP - CASTER_LEVEL);
}

// The ring adds +4 to the save: a roll of 13 fails bare and makes it worn, so
// the reduced dice are halved as well.
TEST_F(FireballDamageTest, ARingAddsFourToTheSave)
{
	Creature& wearer = add_creature(3);
	Creature& bare = add_creature(4);
	wearer.wear(ItemCreator::create("ring_of_fire_resistance", Vector2D{ 3, 5 }, mock.content_registry), EquipmentSlot::RIGHT_RING);
	force_max_dice_then_saves({ 13, 13 });

	burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(wearer.get_hp(), STARTING_HP - (CASTER_LEVEL * 4) / 2);
	EXPECT_EQ(bare.get_hp(), STARTING_HP - MAX_DAMAGE);
}
