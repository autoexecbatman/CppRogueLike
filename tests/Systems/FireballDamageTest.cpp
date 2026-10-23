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

#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Paths.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/SpellSystem.h"
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
			Vector2D{ column, 5 },
			ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK });
		creature->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		creature->set_strength(10);
		creature->set_body_plan(
			{ EquipmentSlot::RIGHT_RING, EquipmentSlot::HEAD });
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
		return SpellSystem::burst_fireball(
			center,
			CASTER_LEVEL,
			RADIUS,
			DamageSource::PLAYER,
			ctx);
	}

	static constexpr int CASTER_LEVEL = 3;
	static constexpr int RADIUS = 2;
	static constexpr int STARTING_HP = 30;
	static constexpr int MAX_DAMAGE = CASTER_LEVEL * 6;

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
	// Outside every burst below, unless a test moves it in.
	std::unique_ptr<Player> caster{ std::make_unique<Player>(
		Vector2D{ 20, 5 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// Fireball is fire: a drunk potion of fire resistance is a ring's worth, -2 a die.
TEST_F(FireballDamageTest, APotionOfFireResistanceTakesTwoOffEveryDie)
{
	Creature& resistant = add_creature(3);
	Creature& exposed = add_creature(4);
	buffs.add_buff(
		resistant,
		BuffType::FIRE_RESISTANCE,
		1,
		10,
		BuffApplication::ADD,
		ctx.gameState->get_time());
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

	const auto burst = SpellSystem::burst_fireball(
		Vector2D{ 3, 5 },
		20,
		RADIUS,
		DamageSource::PLAYER,
		ctx);

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
	wearer.wear(
		ItemCreator::create("ring_of_fire_resistance", Vector2D{ 3, 5 }, ctx),
		EquipmentSlot::RIGHT_RING);
	force_max_dice_then_saves({ 1 });

	burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(wearer.get_hp(), STARTING_HP - CASTER_LEVEL * 4)
		<< "the ring did not take two off each die";
}

// The helm is double strength: -4 on every die.
TEST_F(FireballDamageTest, TheHelmOfBrillianceTakesFourOffEveryDie)
{
	Creature& wearer = add_creature(3);
	wearer.wear(
		ItemCreator::create("helm_of_brilliance", Vector2D{ 3, 5 }, ctx),
		EquipmentSlot::HEAD);
	// A dormant helm protects from nothing, so its word is learned and spoken first.
	Item& helm = *wearer.get_equipped_item(EquipmentSlot::HEAD);
	helm.identify_type();
	speak_command_word(helm, wearer, ctx);
	force_max_dice_then_saves({ 1 });

	burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(wearer.get_hp(), STARTING_HP - CASTER_LEVEL * 2);
}

// A die is never less than 1: three ones with a ring are still three.
TEST_F(FireballDamageTest, ADieNeverGoesBelowOne)
{
	Creature& wearer = add_creature(3);
	wearer.wear(
		ItemCreator::create("ring_of_fire_resistance", Vector2D{ 3, 5 }, ctx),
		EquipmentSlot::RIGHT_RING);
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
	wearer.wear(
		ItemCreator::create("ring_of_fire_resistance", Vector2D{ 3, 5 }, ctx),
		EquipmentSlot::RIGHT_RING);
	force_max_dice_then_saves({ 13, 13 });

	burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(wearer.get_hp(), STARTING_HP - (CASTER_LEVEL * 4) / 2);
	EXPECT_EQ(bare.get_hp(), STARTING_HP - MAX_DAMAGE);
}

// A fireball scroll's damage is the spell's, caster level in d6, so its record
// carries none for the editor to show as real.
TEST_F(FireballDamageTest, TheFireballScrollRecordCarriesNoDamage)
{
	EXPECT_EQ(ctx.itemRegistry->get_params("scroll_fireball").damage, 0);
}

// The Dungeon Master's Guide casts a scroll's spell at never below 6th level,
// so the scroll's burst rolls six dice: six sixes are 36.
TEST_F(FireballDamageTest, TheScrollBurstRollsSixDice)
{
	for (int die = 0; die < 6; ++die)
	{
		mock.dice.set_next_roll(6);
	}

	const auto burst = SpellSystem::burst_fireball(
		Vector2D{ 3, 5 },
		SpellSystem::SCROLL_FIREBALL_CASTER_LEVEL,
		RADIUS,
		DamageSource::PLAYER,
		ctx);

	EXPECT_EQ(burst.diceCount, 6);
	EXPECT_EQ(burst.totalDamage, 36);
}

// The book's burst burns every creature within its radius, and the player is one.
TEST_F(FireballDamageTest, TheBurstBurnsThePlayerInsideItsRadius)
{
	Creature& goblin = add_creature(4);
	caster->position = Vector2D{ 2, 5 };
	const int casterHpBefore = caster->get_hp();
	force_max_dice_then_saves({ 1, 1 });

	const auto burst = burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(burst.struck, 2);
	EXPECT_EQ(burst.playerReach, SpellSystem::PlayerReach::STRUCK);
	EXPECT_EQ(goblin.get_hp(), STARTING_HP - MAX_DAMAGE);
	EXPECT_EQ(caster->get_hp(), casterHpBefore - MAX_DAMAGE);
}

// A player who makes the save takes half, as any creature does.
TEST_F(FireballDamageTest, APlayerWhoSavesTakesHalf)
{
	caster->position = Vector2D{ 2, 5 };
	const int casterHpBefore = caster->get_hp();
	force_max_dice_then_saves({ 20 });

	const auto burst = burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(burst.struck, 1);
	EXPECT_EQ(caster->get_hp(), casterHpBefore - MAX_DAMAGE / 2);
}

// One tile past the radius the player is neither burned nor counted.
TEST_F(FireballDamageTest, ThePlayerOutsideTheRadiusIsUntouched)
{
	add_creature(3);
	caster->position = Vector2D{ 3 + RADIUS + 1, 5 };
	const int casterHpBefore = caster->get_hp();
	force_max_dice_then_saves({ 1 });

	const auto burst = burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(burst.struck, 1);
	EXPECT_EQ(burst.playerReach, SpellSystem::PlayerReach::OUTSIDE);
	EXPECT_EQ(caster->get_hp(), casterHpBefore);
}

// A dead player is passed over, as a dead creature is.
TEST_F(FireballDamageTest, ADeadPlayerIsNeitherBurnedNorCounted)
{
	caster->position = Vector2D{ 2, 5 };
	caster->healthPool->set_hp(0);
	force_max_dice_then_saves({ 1 });

	const auto burst = burst_at(Vector2D{ 3, 5 });

	EXPECT_EQ(burst.struck, 0);
	EXPECT_EQ(burst.playerReach, SpellSystem::PlayerReach::OUTSIDE);
}
