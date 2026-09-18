// file: AttackKindTest.cpp
// Which weapon an attack uses is a property of the attack, not of what the
// attacker happens to be carrying. These pin that: a player with a sword in
// hand and a bow on their back swings the sword when they walk into something,
// and looses the bow only when they aim it.

#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "src/Actor.h"
#include "src/Attacker.h"
#include "src/Item.h"
#include "src/MonsterAttacker.h"
#include "src/PlayerAttacker.h"
#include "src/Player.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Paths.h"
#include "src/Game.h"
#include "src/ShopKeeper.h"

class AttackKindTest : public ::testing::Test
{
protected:
	Game game;
	GameContext ctx;
	std::unique_ptr<Player> player;
	std::unique_ptr<Creature> monster;

	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);

		player = std::make_unique<Player>(Vector2D{ 0, 0 });
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->set_dr(0);
		player->set_thaco(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->healthPool = std::make_unique<HealthPool>(20);
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		player->set_strength(10);
		player->set_dexterity(10);

		monster = std::make_unique<Creature>(Vector2D{ 0, 1 }, ActorData{ TileRef{}, "goblin", 1 });
		monster->experienceReward = std::make_unique<ExperienceReward>(50);
		monster->set_dr(0);
		monster->set_thaco(19);
		monster->armorClass = std::make_unique<ArmorClass>(10);
		monster->healthPool = std::make_unique<HealthPool>(100);
		monster->attacker = std::make_unique<MonsterAttacker>(*monster, DamageInfo{ "1d6", DamageType::PHYSICAL });
		monster->set_strength(8);
		monster->set_dexterity(10);
		monster->set_natural_attack("claws");

		ctx = game.context();
		ctx.playerOwner = &player;

		game.dice.set_test_mode(true);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// A weapon built here rather than loaded, so the test states the two facts it
	// depends on - the name it expects back, and the slot the name belongs to.
	std::unique_ptr<Item> make_weapon(const std::string& name, ItemClass itemClass)
	{
		auto weapon = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, name, 1 });
		weapon->itemClass = itemClass;
		weapon->behavior = Weapon{};
		return weapon;
	}

	void arm_with_sword_and_bow()
	{
		player->equip_item(make_weapon("long sword", ItemClass::SWORD), EquipmentSlot::RIGHT_HAND, ctx);
		player->equip_item(make_weapon("long bow", ItemClass::BOW), EquipmentSlot::MISSILE_WEAPON, ctx);
	}

	// Every part of every finalized attack message, joined. The weapon an attack
	// used is named here, which is the only place the player ever sees it.
	std::string all_attack_text() const
	{
		std::string joined;
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			for (const auto& part : ctx.messageSystem->get_attack_message_at(index))
			{
				joined += part.logMessageText;
			}
		}
		return joined;
	}
};

TEST_F(AttackKindTest, WalkingIntoSomethingSwingsTheWeaponInHand)
{
	arm_with_sword_and_bow();

	game.dice.set_next_d20(20);
	game.dice.set_next_roll(3);
	player->attacker->attack(*monster, AttackKind::MELEE, ctx);

	const std::string text = all_attack_text();
	EXPECT_NE(text.find("long sword"), std::string::npos) << text;
	EXPECT_EQ(text.find("long bow"), std::string::npos) << text;
}

TEST_F(AttackKindTest, AimingLoosesTheMissileWeapon)
{
	arm_with_sword_and_bow();

	game.dice.set_next_d20(20);
	game.dice.set_next_roll(3);
	player->attacker->attack(*monster, AttackKind::RANGED, ctx);

	const std::string text = all_attack_text();
	EXPECT_NE(text.find("long bow"), std::string::npos) << text;
	EXPECT_EQ(text.find("long sword"), std::string::npos) << text;
}

TEST_F(AttackKindTest, WalkingIntoAShopkeeperOpensTradeEvenCarryingABow)
{
	arm_with_sword_and_bow();
	monster->shop = std::make_unique<ShopKeeper>();

	const int hpBefore = monster->get_hp();
	game.dice.set_next_d20(20);
	game.dice.set_next_roll(3);
	player->attacker->attack(*monster, AttackKind::MELEE, ctx);

	// Trading is what a bump on a trader does, so nothing was struck.
	EXPECT_EQ(monster->get_hp(), hpBefore);
	EXPECT_EQ(ctx.messageSystem->get_stored_message_count(), 0u);
}

TEST_F(AttackKindTest, ShootingAShopkeeperIsAnAttackRatherThanAnOffer)
{
	arm_with_sword_and_bow();
	monster->shop = std::make_unique<ShopKeeper>();

	const int hpBefore = monster->get_hp();
	game.dice.set_next_d20(20);
	game.dice.set_next_roll(3);
	player->attacker->attack(*monster, AttackKind::RANGED, ctx);

	EXPECT_LT(monster->get_hp(), hpBefore);
}
