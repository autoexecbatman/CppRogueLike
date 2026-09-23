// file: SpiderVenomTest.cpp
// The web weaver's venom, by the book.
//
// What it is for. The game's three spiders are invented, so each is matched to the
// Monstrous Manual spider it is closest to (PDF pages 1876-1879 of the 2e archive). The
// web weaver - a web-spinner, 1d8 bite, THAC0 17, armour class 5 - is the Manual's giant
// spider, a web-spinner with a 1-8 bite, THAC0 17 and armour class 4: "Their poison is
// Type F, which causes immediate death if the victim fails the saving throw." Table 51
// (Dungeon Master's Guide, PDF page 737) prints Type F as "Death/0": all hit points lost
// on a failure, nothing at all on a success.
//
// Poison is injected "by bite or sting" (PDF page 736), so a bite that never landed
// injects nothing. The saving throw is the victim's against paralyzation, poison and death
// magic, with the Constitution adjustment its own table prints.
//
// Every roll is scripted in the order the game asks for it: the attack roll, the damage
// die, then the save against the venom.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SpiderVenomTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>

#include "src/Actor.h"
#include "src/AiWebSpinner.h"
#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/DataManager.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/MonsterAttacker.h"
#include "src/Player.h"
#include "src/SavingThrow.h"

class SpiderVenomTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		game.dataManager.load_all_data(game.messageSystem);

		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->set_dr(0);
		player->set_strength(10);
		player->set_dexterity(10);
		player->set_constitution(10);

		ctx = game.context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		game.dice.set_test_mode(true);

		// An open corridor, so the spinner beside the player is in view.
		map.init_tiles();
		for (int col = 1; col < 19; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR, 1.0);
		}
		map.compute_fov(ctx);

		// A 1d4 bite from THAC0 20, which needs a 10 against armour class 10.
		spinner.experienceReward = std::make_unique<ExperienceReward>(0);
		spinner.healthPool = std::make_unique<HealthPool>(10);
		spinner.armorClass = std::make_unique<ArmorClass>(5);
		spinner.attacker = std::make_unique<MonsterAttacker>(spinner, DamageInfo{ "1d4", DamageType::PHYSICAL });
		spinner.set_dr(0);
		spinner.set_thaco(20);
		spinner.set_strength(8);
		spinner.set_dexterity(10);
		spinner.set_natural_attack("fangs");
		spinner.ai = std::make_unique<AiWebSpinner>(0);
	}

	void TearDown() override
	{
		game.dice.set_test_mode(false);
		game.dice.clear_fixed_rolls();
	}

	// Queues rolls in the order the game asks for them.
	void script(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			game.dice.set_next_roll(roll);
		}
	}

	// The roll this player needs against the venom, with its Constitution adjustment.
	int venom_save_target()
	{
		const int constitutionBonus = game.dataManager.constitution_for(player->get_constitution()).PoisonSave;
		return SavingThrows::target(player->get_creature_class(), player->get_creature_level(), SavingThrow::PARALYZATION_POISON_DEATH) - constitutionBonus;
	}

	void one_turn()
	{
		spinner.ai->update(spinner, ctx);
	}

	static constexpr int STARTING_HP = 100;
	static constexpr int BITE_DIE = 4;

	Game game;
	GameContext ctx;
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 4, 5 }) };
	Creature spinner{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "web weaver", 0 } };
};

// "Death/0": a failed save takes every hit point, however many were left.
TEST_F(SpiderVenomTest, AFailedSaveAgainstTheVenomKills)
{
	script({ 20, BITE_DIE, 1 });

	one_turn();

	EXPECT_TRUE(player->is_dead()) << "the venom of a Type F poison did not kill";
	EXPECT_EQ(player->get_hp(), 0);
}

// "Death/0": a made save takes nothing at all, so only the bite is felt.
TEST_F(SpiderVenomTest, AMadeSaveAgainstTheVenomTakesNothing)
{
	script({ 20, BITE_DIE, 20 });

	one_turn();

	EXPECT_FALSE(player->is_dead());
	EXPECT_EQ(STARTING_HP - player->get_hp(), BITE_DIE) << "the venom was felt through a made save";
}

// Injected by the bite: one that misses injects nothing, so the 1 queued for a save that
// would kill is never read.
TEST_F(SpiderVenomTest, ABiteThatMissesInjectsNothing)
{
	script({ 1, BITE_DIE, 1 });

	one_turn();

	EXPECT_FALSE(player->is_dead()) << "venom from a bite that never landed";
	EXPECT_EQ(player->get_hp(), STARTING_HP);
}

// The Constitution table's poison adjustment counts: 25 gives +4, so a roll four short of
// the bare target still makes the save.
TEST_F(SpiderVenomTest, TheVenomSaveTakesTheConstitutionAdjustment)
{
	player->set_constitution(25);
	ASSERT_EQ(game.dataManager.constitution_for(25).PoisonSave, 4);
	const int needed = venom_save_target();
	ASSERT_GT(needed, 1);
	script({ 20, BITE_DIE, needed });

	one_turn();

	EXPECT_FALSE(player->is_dead()) << "the Constitution adjustment was left out of the save";
}

// The same roll from a Constitution 10 victim, which gets no adjustment, is a failure.
TEST_F(SpiderVenomTest, TheSameRollWithoutTheAdjustmentFails)
{
	const int needed = venom_save_target();
	ASSERT_EQ(game.dataManager.constitution_for(10).PoisonSave, 0);
	script({ 20, BITE_DIE, needed - 1 });

	one_turn();

	EXPECT_TRUE(player->is_dead()) << "a roll under the target made the save";
}
