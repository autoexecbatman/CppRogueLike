// file: TypedAttackResistanceTest.cpp
// A monster's fire or cold attack meets the target's resistance the way the
// Dungeon Master's Guide says a ring does: fire dice "are calculated at -2 per
// die, but each die is never less than 1", cold dice at -1 per die; a bonus on
// the roll is not a die and is not reduced. A bite offers no saving throw, so
// the ring's save bonus has nothing to add here.
//
// The defect this pins: fire and cold were resisted only inside the fireball,
// and five monsters - chimera, dragon, fire wolf, pit fiend, ice wolf - dealt
// typed damage through Attacker as a total rolled from min to max, which the
// resolver then handed straight back. The ring was worn and did nothing.
//
// Expected values: with the attack roll forced to hit, a fire wolf's 1d6 forced
// to 6 through a ring is 4; an ice wolf's 6 through a ring of warmth is 5; a
// chimera's 3d12 at 12, 12, 12 through the helm is 8 + 8 + 8; a dragon's
// 1d12+5 at 12 through a ring is 10 + 5.

#include <gtest/gtest.h>

#include "src/Creature.h"
#include "src/EquipmentSlot.h"
#include "src/MonsterAttacker.h"
#include "src/Player.h"
#include "src/AttackKind.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/Paths.h"
#include "src/ItemCreator.h"

class TypedAttackResistanceTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);
		game.itemRegistry.load(Paths::ITEMS);

		player = std::make_unique<Player>(Vector2D{ 0, 0 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->set_dr(0);
		player->set_thaco(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->set_strength(10);
		player->set_dexterity(10);

		ctx = game.context();
		ctx.playerOwner = &player;
		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// A monster whose natural attack is the given damage, strong enough to hit.
	std::unique_ptr<Creature> monster_with(DamageInfo attack)
	{
		auto monster = std::make_unique<Creature>(Vector2D{ 0, 1 }, ActorData{ TileRef{}, "beast", 1 });
		monster->experienceReward = std::make_unique<ExperienceReward>(0);
		monster->set_dr(0);
		monster->set_thaco(19);
		monster->armorClass = std::make_unique<ArmorClass>(6);
		monster->healthPool = std::make_unique<HealthPool>(10);
		monster->attacker = std::make_unique<MonsterAttacker>(*monster, attack);
		// Strength 8 carries no damage adjustment, so the dice are all that lands.
		monster->set_strength(8);
		monster->set_dexterity(10);
		monster->set_natural_attack("bite");
		return monster;
	}

	void wear(std::string_view key, EquipmentSlot slot)
	{
		player->equip_item(ItemCreator::create(key, Vector2D{ 0, 0 }, ctx), slot, ctx);
	}

	// The attack roll first, always a hit, then the damage dice in order.
	void force_hit_then_dice(std::initializer_list<int> dice)
	{
		game.dice.set_next_roll(20);
		for (const int die : dice)
		{
			game.dice.set_next_roll(die);
		}
	}

	int hp_lost_after(Creature& monster)
	{
		monster.attacker->attack(*player, AttackKind::MELEE, ctx);
		return STARTING_HP - player->get_hp();
	}

	static constexpr int STARTING_HP = 100;

	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player;
};

// A fire wolf's bite through a ring of fire resistance: 6 becomes 4.
TEST_F(TypedAttackResistanceTest, ARingTakesTwoOffAFireBite)
{
	wear("ring_of_fire_resistance", EquipmentSlot::RIGHT_RING);
	auto wolf = monster_with(DamageInfo{ "1d6", DamageType::FIRE });
	force_hit_then_dice({ 6 });

	EXPECT_EQ(hp_lost_after(*wolf), 4) << "the ring was worn and the bite landed whole";
}

// An ice wolf's bite through a ring of warmth: 6 becomes 5.
TEST_F(TypedAttackResistanceTest, ARingOfWarmthTakesOneOffAColdBite)
{
	wear("ring_of_cold_resistance", EquipmentSlot::LEFT_RING);
	auto wolf = monster_with(DamageInfo{ "1d6", DamageType::COLD });
	force_hit_then_dice({ 6 });

	EXPECT_EQ(hp_lost_after(*wolf), 5);
}

// A chimera's 3d12 through the helm: every die loses four.
TEST_F(TypedAttackResistanceTest, TheHelmTakesFourOffEveryDieOfAChimera)
{
	wear("helm_of_brilliance", EquipmentSlot::HEAD);
	auto chimera = monster_with(DamageInfo{ "3d12", DamageType::FIRE });
	force_hit_then_dice({ 12, 12, 12 });

	EXPECT_EQ(hp_lost_after(*chimera), 24);
}

// A dragon's 1d12+5: the die is reduced, the +5 is not a die.
TEST_F(TypedAttackResistanceTest, ABonusOnTheRollIsNotADie)
{
	wear("ring_of_fire_resistance", EquipmentSlot::RIGHT_RING);
	auto dragon = monster_with(DamageInfo{ "1d12+5", DamageType::FIRE });
	force_hit_then_dice({ 12 });

	EXPECT_EQ(hp_lost_after(*dragon), 15);
}

// A physical bite is untouched by a ring of fire resistance.
TEST_F(TypedAttackResistanceTest, APhysicalBiteIgnoresTheRing)
{
	wear("ring_of_fire_resistance", EquipmentSlot::RIGHT_RING);
	auto wolf = monster_with(DamageInfo{ "1d6", DamageType::PHYSICAL });
	force_hit_then_dice({ 6 });

	EXPECT_EQ(hp_lost_after(*wolf), 6);
}

// Bare, a fire bite lands whole: three ones stay three.
TEST_F(TypedAttackResistanceTest, WithoutResistanceTheDiceLandAsRolled)
{
	auto chimera = monster_with(DamageInfo{ "3d12", DamageType::FIRE });
	force_hit_then_dice({ 1, 1, 1 });

	EXPECT_EQ(hp_lost_after(*chimera), 3);
}
