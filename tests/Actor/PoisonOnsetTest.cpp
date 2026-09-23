// file: PoisonOnsetTest.cpp
// Poison that takes hold later, and the game's giant spider, which carries it.
//
// What it is for. Every poison in Table 51 has an onset (Dungeon Master's Guide, PDF page
// 737 of the 2e archive), so the bite is long over when the poison lands. A creature
// carries one dose: what it will cost, and the rounds left before it arrives.
//
// The game's giant spider - a hunting spider with a 1d6 bite and THAC0 19 - is the
// Monstrous Manual's huge spider (PDF page 1878), which hunts, bites for 1-6 and has
// THAC0 19. "Huge spiders also posses Type A poison, with the same effects as that of a
// large spider. Victims receive a +1 to saving throws vs. the poison of the huge spider",
// and the large spider's is "Type A, the onset time is 15 minutes. Victims take 15 points
// of damage, or no damage if a saving throw vs. poison ... is successful." A round is a
// minute and a game turn is a round, so the fifteen points land fifteen rounds later.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PoisonOnsetTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>

#include <nlohmann/json.hpp>

#include "src/Actor.h"
#include "src/AiGiantSpider.h"
#include "src/ArmorClass.h"
#include "src/ConstitutionAttributes.h"
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

class PoisonOnsetTest : public ::testing::Test
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

		// An open corridor, so the spider beside the player is in view.
		map.init_tiles();
		for (int col = 1; col < 19; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR, 1.0);
		}
		map.compute_fov(ctx);

		// A 1d6 bite from THAC0 20, which needs a 10 against armour class 10.
		spider.experienceReward = std::make_unique<ExperienceReward>(0);
		spider.healthPool = std::make_unique<HealthPool>(10);
		spider.armorClass = std::make_unique<ArmorClass>(5);
		spider.attacker = std::make_unique<MonsterAttacker>(spider, DamageInfo{ "1d6", DamageType::PHYSICAL });
		spider.set_dr(0);
		spider.set_thaco(20);
		spider.set_strength(8);
		spider.set_dexterity(10);
		spider.set_natural_attack("fangs");
		spider.ai = std::make_unique<AiGiantSpider>(0);
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

	// Runs that many rounds of the poison working, and returns the hit points it took.
	int tick_through(Creature& victim, int rounds)
	{
		const int before = victim.get_hp();
		for (int round = 0; round < rounds; ++round)
		{
			victim.tick_poison(ctx);
		}
		return before - victim.get_hp();
	}

	// The roll this player needs against poison, with its Constitution adjustment.
	int venom_save_target()
	{
		const int constitutionBonus = game.dataManager.constitution_for(player->get_constitution()).PoisonSave;
		return SavingThrows::target(player->get_creature_class(), player->get_creature_level(), SavingThrow::PARALYZATION_POISON_DEATH) - constitutionBonus;
	}

	static constexpr int STARTING_HP = 100;
	static constexpr int BITE_DIE = 6;
	static constexpr int TYPE_A_ONSET_ROUNDS = 15;
	static constexpr int TYPE_A_DAMAGE = 15;

	Game game;
	GameContext ctx;
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 4, 5 }) };
	Creature spider{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "giant spider", 0 } };
};

// A dose waits its rounds out, and lands on the last of them.
TEST_F(PoisonOnsetTest, ADoseLandsWhenItsRoundsRunOut)
{
	player->take_poison(TYPE_A_ONSET_ROUNDS, TYPE_A_DAMAGE);

	EXPECT_EQ(tick_through(*player, TYPE_A_ONSET_ROUNDS - 1), 0) << "the poison landed early";
	EXPECT_EQ(tick_through(*player, 1), TYPE_A_DAMAGE);
	EXPECT_FALSE(player->get_pending_poison().has_value()) << "the dose is still working after it landed";
}

// It lands once, not every round after.
TEST_F(PoisonOnsetTest, ADoseLandsOnlyOnce)
{
	player->take_poison(1, TYPE_A_DAMAGE);

	EXPECT_EQ(tick_through(*player, 20), TYPE_A_DAMAGE);
}

// One dose at a time: a second bite replaces what the first left working.
TEST_F(PoisonOnsetTest, ASecondDoseReplacesTheFirst)
{
	player->take_poison(10, 5);
	player->take_poison(2, 7);

	EXPECT_EQ(tick_through(*player, 2), 7);
	EXPECT_EQ(tick_through(*player, 20), 0);
}

// A poison that kills does what any other damage does.
TEST_F(PoisonOnsetTest, ADoseCanKill)
{
	player->set_hp(4);
	player->take_poison(1, TYPE_A_DAMAGE);

	tick_through(*player, 1);

	EXPECT_TRUE(player->is_dead());
}

// A dose survives a saved game: what is owed is owed.
TEST_F(PoisonOnsetTest, ADoseSurvivesASaveAndALoad)
{
	player->take_poison(9, TYPE_A_DAMAGE);

	nlohmann::json saved;
	player->save(saved);
	auto loaded = std::make_unique<Player>(Vector2D{ 0, 0 });
	loaded->healthPool = std::make_unique<HealthPool>(STARTING_HP);
	loaded->load(saved);

	ASSERT_TRUE(loaded->get_pending_poison().has_value()) << "the poison was forgotten in the save";
	EXPECT_EQ(loaded->get_pending_poison()->roundsUntilOnset, 9);
	EXPECT_EQ(loaded->get_pending_poison()->damage, TYPE_A_DAMAGE);
}

// A creature carrying nothing ticks nothing.
TEST_F(PoisonOnsetTest, WithoutADoseNothingHappens)
{
	EXPECT_EQ(tick_through(*player, 30), 0);
	EXPECT_FALSE(player->get_pending_poison().has_value());
}

// The dead are not poisoned further: the dose stays where it is, and nothing lands.
TEST_F(PoisonOnsetTest, ADeadCreatureIsLeftAlone)
{
	Creature victim{ Vector2D{ 7, 5 }, ActorData{ TileRef{}, "victim", 0 } };
	victim.healthPool = std::make_unique<HealthPool>(10);
	victim.take_poison(1, TYPE_A_DAMAGE);
	victim.set_hp(0);

	tick_through(victim, 20);

	ASSERT_TRUE(victim.get_pending_poison().has_value()) << "a dead creature took its dose";
	EXPECT_EQ(victim.get_pending_poison()->roundsUntilOnset, 1) << "the dose counted down in a dead creature";
}

// The bite: a failed save costs nothing at the time, and fifteen points fifteen rounds on.
TEST_F(PoisonOnsetTest, AFailedSaveAgainstAGiantSpiderPaysFifteenRoundsLater)
{
	script({ 20, BITE_DIE, 1 });

	spider.ai->update(spider, ctx);

	EXPECT_EQ(STARTING_HP - player->get_hp(), BITE_DIE) << "the venom landed with the bite";
	ASSERT_TRUE(player->get_pending_poison().has_value()) << "no poison was left working";
	EXPECT_EQ(tick_through(*player, TYPE_A_ONSET_ROUNDS - 1), 0) << "the fifteen minutes were not waited out";
	EXPECT_EQ(tick_through(*player, 1), TYPE_A_DAMAGE);
}

// "No damage if a saving throw vs. poison is successful": a made save leaves nothing behind.
TEST_F(PoisonOnsetTest, AMadeSaveAgainstAGiantSpiderLeavesNoPoison)
{
	script({ 20, BITE_DIE, 20 });

	spider.ai->update(spider, ctx);

	EXPECT_FALSE(player->get_pending_poison().has_value()) << "a made save still poisoned";
	EXPECT_EQ(tick_through(*player, 30), 0);
}

// "Victims receive a +1 to saving throws vs. the poison of the huge spider": a roll one
// short of the bare target still makes it.
TEST_F(PoisonOnsetTest, TheHugeSpidersVictimSavesAtPlusOne)
{
	const int needed = venom_save_target();
	script({ 20, BITE_DIE, needed - 1 });

	spider.ai->update(spider, ctx);

	EXPECT_FALSE(player->get_pending_poison().has_value()) << "the spider's own +1 was left out";
}

// Two short of it fails, so the +1 is one point and not a licence.
TEST_F(PoisonOnsetTest, TwoShortOfTheTargetStillFails)
{
	const int needed = venom_save_target();
	script({ 20, BITE_DIE, needed - 2 });

	spider.ai->update(spider, ctx);

	EXPECT_TRUE(player->get_pending_poison().has_value()) << "a roll two under the target saved";
}
