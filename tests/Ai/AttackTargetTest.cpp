// file: AttackTargetTest.cpp
// A monster's hunting step strikes the creature it was handed, not the player.
//
// What it is for. AiMonster::move_or_attack and the ranged AI's shot were each given a
// target, then struck the player whoever they had been given. Every caller passes the
// player today, so play never saw it; a decoy shows it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=AttackTargetTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/AiMonster.h"
#include "src/AiMonsterRanged.h"
#include "src/AiSpider.h"
#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/EquipmentSlot.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/ItemCreator.h"
#include "src/Map.h"
#include "src/MonsterAttacker.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/RandomDice.h"
#include "tests/mocks/TestDice.h"

namespace
{

constexpr int STARTING_HP = 100;
constexpr int CORRIDOR_ROW = 5;
// The player is 8 tiles from the hunter: out of reach of a blow, and out of a bow's 5.
constexpr Vector2D PLAYER_TILE{ 2, CORRIDOR_ROW };
constexpr Vector2D HUNTER_TILE{ 10, CORRIDOR_ROW };
// Beside the hunter, for a blow.
constexpr Vector2D ADJACENT_DECOY_TILE{ 11, CORRIDOR_ROW };
// Three tiles off down the corridor, inside a bow's range and in plain sight.
constexpr Vector2D DISTANT_DECOY_TILE{ 13, CORRIDOR_ROW };

// The hunting steps, public for the test.
class MeleeHunter final : public AiMonster
{
public:
	using AiMonster::move_or_attack;
};

class RangedHunter final : public AiMonsterRanged
{
public:
	using AiMonsterRanged::move_or_attack;
};

class SpiderHunter final : public AiSpider
{
public:
	using AiSpider::move_or_attack;
};

// Gives a creature what a blow or a shot reads from it.
void make_combatant(Creature& creature)
{
	creature.experienceReward = std::make_unique<ExperienceReward>(0);
	creature.healthPool = std::make_unique<HealthPool>(STARTING_HP);
	creature.armorClass = std::make_unique<ArmorClass>(10);
	creature.set_dr(0);
	creature.set_thaco(20);
	creature.set_strength(10);
	creature.set_dexterity(10);
}

} // namespace

class AttackTargetTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);
		game.tileConfig.load(Paths::TILE_CONFIG);
		game.itemRegistry.load(Paths::ITEMS);

		make_combatant(*player);
		player->attacker = std::make_unique<PlayerAttacker>(*player);
		make_combatant(hunter);
		hunter.attacker = std::make_unique<MonsterAttacker>(hunter, DamageInfo{ "1d4", DamageType::PHYSICAL });
		hunter.set_natural_attack("fist");
		hunter.set_body_plan({ EquipmentSlot::RIGHT_HAND, EquipmentSlot::MISSILE_WEAPON });
		make_combatant(decoy);
		decoy.attacker = std::make_unique<MonsterAttacker>(decoy, DamageInfo{ "1d4", DamageType::PHYSICAL });

		ctx = game.context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		ctx.creatures = &creatures;

		map.init_tiles();
		for (int column = 1; column < 19; ++column)
		{
			map.set_tile(Vector2D{ column, CORRIDOR_ROW }, TileType::FLOOR);
		}
		map.compute_fov(ctx);

		// The top of every die: the morale check holds and every blow lands.
		game.dice.set_test_mode(true);
		for (int queued = 0; queued < 40; ++queued)
		{
			game.dice.set_next_roll(RandomDice::HIGHEST);
		}
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	Game game{ TEST_DICE_SEED };
	GameContext ctx;
	Map map{ 20, 20 };
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(PLAYER_TILE) };
	Creature hunter{ HUNTER_TILE, ActorData{ TileRef{}, "hunter", ColorPairId::WHITE_BLACK } };
	Creature decoy{ ADJACENT_DECOY_TILE, ActorData{ TileRef{}, "decoy", ColorPairId::WHITE_BLACK } };
};

TEST_F(AttackTargetTest, AMeleeHunterStrikesTheCreatureItWasHanded)
{
	MeleeHunter ai{};

	ai.move_or_attack(hunter, decoy, ctx);

	EXPECT_LT(decoy.get_hp(), STARTING_HP) << "the creature handed over was not struck";
	EXPECT_EQ(player->get_hp(), STARTING_HP) << "the player was struck from eight tiles away";
}

TEST_F(AttackTargetTest, ASpiderBitesTheCreatureItWasHanded)
{
	SpiderHunter ai{};

	ai.move_or_attack(hunter, decoy, ctx);

	EXPECT_LT(decoy.get_hp(), STARTING_HP) << "the creature handed over was not bitten";
	EXPECT_EQ(player->get_hp(), STARTING_HP) << "the player was bitten from eight tiles away";
}

TEST_F(AttackTargetTest, ARangedHunterShootsTheCreatureItWasHanded)
{
	RangedHunter ai{};
	hunter.wear(ItemCreator::create("long_bow", hunter.position, ctx), EquipmentSlot::MISSILE_WEAPON);
	decoy.position = DISTANT_DECOY_TILE;

	ai.move_or_attack(hunter, decoy, ctx);

	EXPECT_LT(decoy.get_hp(), STARTING_HP) << "the creature handed over was not shot";
	EXPECT_EQ(player->get_hp(), STARTING_HP) << "the player was shot from eight tiles away";
}
