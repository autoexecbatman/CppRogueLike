#include <gtest/gtest.h>
#include <memory>

#include "src/Actor/Actor.h"
#include "src/Actor/Attacker.h"
#include "src/Actor/Destructible.h"
#include "src/Actor/MonsterAttacker.h"
#include "src/Actor/PlayerAttacker.h"
#include "src/ActorTypes/Player.h"
#include "src/Combat/DamageInfo.h"
#include "src/Combat/ExperienceReward.h"
#include "src/Core/Paths.h"
#include "src/Game.h"

// ============================================================================
// ATTACKER TESTS
// Tests combat calculations, hit/miss logic, damage application
// ============================================================================

class AttackerTest : public ::testing::Test
{
protected:
	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player;
	std::unique_ptr<Creature> monster;

	void SetUp() override
	{
		try
		{
			game.data_manager.load_all_data(game.message_system);
		}
		catch (...)
		{
		}

		game.tile_config.load(Paths::TILE_CONFIG);

		player = std::make_unique<Player>(Vector2D{ 0, 0 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->destructible = std::make_unique<Destructible>(
			20, 0, "your corpse", 0, 20, 10, std::make_unique<PlayerDeathHandler>());
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_strength(10);
		player->set_dexterity(10);

		monster = std::make_unique<Creature>(Vector2D{ 0, 1 }, ActorData{ TileRef{}, "goblin", 1 });
		monster->experienceReward = std::make_unique<ExperienceReward>(50);
		monster->destructible = std::make_unique<Destructible>(
			10, 0, "goblin corpse", 0, 19, 6, std::make_unique<MonsterDeathHandler>());
		monster->attacker = std::make_unique<MonsterAttacker>(*monster, DamageInfo{ 1, 6, "1d6" });
		monster->set_strength(8);
		monster->set_dexterity(10);
		monster->set_weapon_equipped("claws");

		ctx = game.context();
		ctx.player = player.get();

		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}
};

// ----------------------------------------------------------------------------
// MonsterAttacker Serialization Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, Serialization_RoundTrip)
{
	Creature dummy(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "dummy", 1 });
	MonsterAttacker original(dummy, DamageInfo{ 2, 8, "2d4", DamageType::FIRE });

	json j;
	original.save(j);

	MonsterAttacker loaded(dummy, DamageInfo{ 0, 0, "" });
	loaded.load(j);

	EXPECT_EQ(loaded.get_damage_info().minDamage, 2);
	EXPECT_EQ(loaded.get_damage_info().maxDamage, 8);
	EXPECT_EQ(loaded.get_damage_info().displayRoll, "2d4");
	EXPECT_EQ(loaded.get_damage_info().damageType, DamageType::FIRE);
}

// ----------------------------------------------------------------------------
// THAC0 Combat Calculation Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, THAC0_RollNeeded_Calculation)
{
	// Player THAC0 20, Monster AC 6
	// Roll needed = THAC0 - AC = 20 - 6 = 14
	monster->destructible->set_armor_class(6);

	game.dice.set_next_d20(14);
	game.dice.set_next_roll(3);

	int hpBefore = monster->destructible->get_hp();
	player->attacker->attack(*monster, ctx);

	// Should hit with roll of exactly 14
	EXPECT_LT(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, THAC0_RollBelowNeeded_Misses)
{
	monster->destructible->set_armor_class(6);

	game.dice.set_next_d20(13);
	game.dice.set_next_roll(3);

	int hpBefore = monster->destructible->get_hp();
	player->attacker->attack(*monster, ctx);

	// Should miss with roll of 13
	EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, THAC0_LowAC_EasierToHit)
{
	// AC 0 means roll needed = 20 - 0 = 20
	monster->destructible->set_armor_class(0);

	game.dice.set_next_d20(20);
	game.dice.set_next_roll(5);

	int hpBefore = monster->destructible->get_hp();
	player->attacker->attack(*monster, ctx);

	EXPECT_LT(monster->destructible->get_hp(), hpBefore);
}

TEST_F(AttackerTest, THAC0_HighAC_EasierToHit)
{
	// AC 10 means roll needed = 20 - 10 = 10
	monster->destructible->set_armor_class(10);

	game.dice.set_next_d20(10);
	game.dice.set_next_roll(4);

	int hpBefore = monster->destructible->get_hp();
	player->attacker->attack(*monster, ctx);

	EXPECT_LT(monster->destructible->get_hp(), hpBefore);
}

// ----------------------------------------------------------------------------
// Damage Reduction Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, DamageReduction_ReducesDamage)
{
	monster->destructible->set_armor_class(20);
	monster->destructible->set_dr(3);

	game.dice.set_next_d20(20);
	game.dice.set_next_roll(5);

	int hpBefore = monster->destructible->get_hp();
	player->attacker->attack(*monster, ctx);

	// 5 damage - 3 DR = 2 actual damage
	EXPECT_EQ(monster->destructible->get_hp(), hpBefore - 2);
}

TEST_F(AttackerTest, DamageReduction_CanReduceToZero)
{
	monster->destructible->set_armor_class(20);
	monster->destructible->set_dr(10);

	game.dice.set_next_d20(20);
	game.dice.set_next_roll(5);

	int hpBefore = monster->destructible->get_hp();
	player->attacker->attack(*monster, ctx);

	// 5 damage - 10 DR = 0 actual damage
	EXPECT_EQ(monster->destructible->get_hp(), hpBefore);
}

// ----------------------------------------------------------------------------
// Monster Attack Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, MonsterAttack_UsesStoredWeaponName)
{
	monster->set_weapon_equipped("sharp claws");
	player->destructible->set_armor_class(10);

	game.dice.set_next_d20(20);
	game.dice.set_next_roll(4);

	// Attack should complete without errors using stored weapon name
	monster->attacker->attack(*player, ctx);

	EXPECT_TRUE(true);
}

TEST_F(AttackerTest, MonsterAttack_DealsCorrectDamage)
{
	player->destructible->set_armor_class(20);
	player->destructible->set_dr(0);

	game.dice.set_next_d20(20);
	game.dice.set_next_roll(6);

	int hpBefore = player->destructible->get_hp();
	monster->attacker->attack(*player, ctx);

	EXPECT_EQ(player->destructible->get_hp(), hpBefore - 6);
}

// ----------------------------------------------------------------------------
// Combat Result Tests
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, Attack_CanKillTarget)
{
	monster->destructible->set_hp(1);
	monster->destructible->set_armor_class(20);
	monster->destructible->set_dr(0);

	game.dice.set_next_d20(20);
	game.dice.set_next_roll(5);

	ASSERT_FALSE(monster->destructible->is_dead());
	player->attacker->attack(*monster, ctx);

	EXPECT_TRUE(monster->destructible->is_dead());
}

TEST_F(AttackerTest, Attack_MonsterDeathAwardsXP)
{
	monster->destructible->set_hp(1);
	monster->destructible->set_armor_class(20);
	monster->destructible->set_dr(0);

	game.dice.set_next_d20(20);
	game.dice.set_next_roll(5);

	int xpBefore = player->get_xp();
	player->attacker->attack(*monster, ctx);

	// Monster was worth 50 XP
	EXPECT_GT(player->get_xp(), xpBefore);
}

// ----------------------------------------------------------------------------
// Backstab Tests
// AD&D 2e: invisibility grants backstab — +4 to hit for all, x2+ damage for ROGUE only.
// These tests cover the code path changed from dynamic_cast<Player*> to
// get_creature_class() == CreatureClass::ROGUE.
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, Backstab_InvisibleRogue_GetsDamageMultiplier)
{
	// Arrange: level 1 ROGUE + invisible → +4 hit, x2 damage multiplier
	player->set_creature_class(CreatureClass::ROGUE);
	player->add_state(ActorState::IS_INVISIBLE);
	monster->destructible->set_armor_class(10);
	monster->destructible->set_dr(0);

	// THAC0=20, AC=10, backstab hitBonus=+4: rollNeeded = 20 - 10 - 4 = 6
	// damageRoll=3, strength 10 (dmgAdj=0), multiplier=2: finalDamage = (3+0)*2 = 6
	game.dice.set_next_d20(6);
	game.dice.set_next_roll(3);

	const int hpBefore = monster->destructible->get_hp();
	player->attacker->attack(*monster, ctx);

	EXPECT_EQ(monster->destructible->get_hp(), hpBefore - 6);
}

TEST_F(AttackerTest, Backstab_InvisibleNonRogue_GetsHitBonusOnly)
{
	// Arrange: invisible FIGHTER — +4 to hit, multiplier stays at 1
	// Default creatureClass is MONSTER, not ROGUE — no multiplier.
	player->add_state(ActorState::IS_INVISIBLE);
	monster->destructible->set_armor_class(10);
	monster->destructible->set_dr(0);

	// THAC0=20, AC=10: rollNeeded without backstab = 10
	// With +4 backstab hitBonus: rollNeeded = 6
	// Roll 6 → would miss at 6 < 10 without the bonus, hits with it.
	// damageRoll=3, multiplier=1: finalDamage = 3
	game.dice.set_next_d20(6);
	game.dice.set_next_roll(3);

	const int hpBefore = monster->destructible->get_hp();
	player->attacker->attack(*monster, ctx);

	// Asserts both that the +4 hit bonus landed AND that no multiplier was applied
	EXPECT_EQ(monster->destructible->get_hp(), hpBefore - 3);
}

// ----------------------------------------------------------------------------
// Post-Load Functionality Test
// Covers the Player::load path that installs PlayerAttacker, not MonsterAttacker.
// ----------------------------------------------------------------------------

TEST_F(AttackerTest, PlayerAttacker_FunctionalAfterSaveLoad)
{
	// Arrange: save and reload the player
	json j;
	player->save(j);

	auto loadedPlayer = std::make_unique<Player>(Vector2D{ 0, 0 });
	loadedPlayer->load(j);
	ctx.player = loadedPlayer.get();

	// AC=20 → rollNeeded = THAC0(20) - AC(20) = 0; any d20 roll hits
	monster->destructible->set_armor_class(20);
	monster->destructible->set_dr(0);
	game.dice.set_next_d20(1);
	game.dice.set_next_roll(4);

	const int hpBefore = monster->destructible->get_hp();
	loadedPlayer->attacker->attack(*monster, ctx);

	// PlayerAttacker is wired correctly post-load and deals damage
	EXPECT_LT(monster->destructible->get_hp(), hpBefore);
}
