// Checks what a hidden spider does in the turn the player discovers it beside them.
//
// What it is for. The Player's Handbook, PDF pages of the 2e archive: "If the ambush
// succeeds, the ambushing group gets its initial attack and the other group must roll
// for surprise in the next round" (224). The initial attack is the spider's normal
// bite - an attack roll against armour class, the bite's own dice, and no strike at all
// against a player whose Sanctuary the spider fails to save against. The surprise roll
// is 1d10, surprised on 1 to 3 (223), with the Dexterity reaction adjustment added (31).
// A surprised player takes one more round of the spider's attacks, at +1 to hit (Table
// 51, page 181) and without any armour class bonus for high Dexterity (224).
//
// Every roll is scripted in the order the game asks for it: the sanctuary save if the
// player is protected; the attack roll, the damage die and the poison roll of the first
// bite; the surprise roll; then the same three for the surprise bite. The spider's
// poison chance is zero, so no venom lands on top of a bite.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SpiderAmbushTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>

#include "src/Actor.h"
#include "src/AiSpider.h"
#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/MonsterAttacker.h"
#include "src/Player.h"
#include "src/SavingThrow.h"

class SpiderAmbushTest : public ::testing::Test
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

		// A 1d4 bite from THAC0 20; strength 8 adds nothing to the damage.
		spider.experienceReward = std::make_unique<ExperienceReward>(0);
		spider.healthPool = std::make_unique<HealthPool>(10);
		spider.armorClass = std::make_unique<ArmorClass>(7);
		spider.attacker = std::make_unique<MonsterAttacker>(spider, DamageInfo{ "1d4", DamageType::PHYSICAL });
		spider.set_dr(0);
		spider.set_thaco(20);
		spider.set_strength(8);
		spider.set_dexterity(10);
		spider.set_natural_attack("fangs");

		// Lying in wait, with turns left, and no venom.
		auto ambusher = std::make_unique<AiSpider>(0);
		ambusher->load(json{ { "isAmbushing", true }, { "ambushCounter", 5 }, { "poisonChance", 0 } });
		spider.ai = std::move(ambusher);
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

	int hp_lost_to_the_ambush()
	{
		spider.ai->update(spider, ctx);
		return STARTING_HP - player->get_hp();
	}

	static constexpr int STARTING_HP = 100;

	Game game;
	GameContext ctx;
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 4, 5 }) };
	Creature spider{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "small spider", 0 } };
};

// The first bite hits for four and a surprise roll of 10 leaves the player ready. A
// second bite, taken with or without that roll, finds a hit among the rolls after it.
TEST_F(SpiderAmbushTest, AnAmbushThePlayerIsReadyForIsOneBite)
{
	script({ 20, 4, 100, 10, 20, 4, 100 });

	EXPECT_EQ(hp_lost_to_the_ambush(), 4) << "1d4 at four, once";
}

// Surprised on a 1: the first bite lands four and the surprise bite three.
TEST_F(SpiderAmbushTest, ASurprisedPlayerTakesASecondBite)
{
	script({ 20, 4, 100, 1, 20, 3, 100 });

	EXPECT_EQ(hp_lost_to_the_ambush(), 7) << "four, then three in the surprise round";
}

// THAC0 20 against armour class 10 needs a 10. The surprise bite's 9 hits only at +1.
TEST_F(SpiderAmbushTest, TheSurpriseBiteIsPlusOneToHit)
{
	script({ 20, 4, 100, 1, 9, 3, 100 });

	EXPECT_EQ(hp_lost_to_the_ambush(), 7) << "a 9 hits a surprised defender";
}

// Dexterity 18 takes four off armour class 10, and its reaction adjustment of +2 still
// leaves a 1 surprised. Surprised, the player stands at 10 again, so with +1 a 9 hits;
// at 6 it would need a 13.
TEST_F(SpiderAmbushTest, ASurprisedPlayerLosesTheDexterityBonusToArmourClass)
{
	player->set_dexterity(18);
	player->update_armor_class(ctx);
	ASSERT_EQ(player->get_armor_class(), 6);
	script({ 20, 4, 100, 1, 9, 3, 100 });

	EXPECT_EQ(hp_lost_to_the_ambush(), 7) << "the surprise bite met the Dexterity bonus";
}

// Dexterity 5 takes 1 off the surprise roll, so a 4 is surprised, and adds 2 to armour
// class 10. Surprise keeps a penalty: the player stands at 12, where with +1 an 8 hits;
// at 10 it would need a 9.
TEST_F(SpiderAmbushTest, LowDexterityIsSurprisedMoreAndKeepsItsPenalty)
{
	player->set_dexterity(5);
	player->update_armor_class(ctx);
	ASSERT_EQ(player->get_armor_class(), 12);
	script({ 20, 4, 100, 4, 8, 3, 100 });

	EXPECT_EQ(hp_lost_to_the_ambush(), 7) << "surprised on a 4, bitten at armour class 12";
}

// Dexterity 17 adds 2 to the surprise roll: a 2 becomes a 4, and the player is ready.
TEST_F(SpiderAmbushTest, HighDexterityCanSpareThePlayerTheSurprise)
{
	player->set_dexterity(17);
	script({ 20, 4, 100, 2, 20, 3, 100 });

	EXPECT_EQ(hp_lost_to_the_ambush(), 4) << "a 2 with +2 is not surprised";
}

// A spider already in the open, beside the player, bites once: only an ambush brings a
// surprise roll, so the 1 queued after the first bite is never read.
TEST_F(SpiderAmbushTest, AnOrdinaryBiteBringsNoSurpriseRoll)
{
	spider.ai->load(json{ { "isAmbushing", false }, { "ambushCounter", 0 }, { "poisonChance", 0 } });
	script({ 20, 4, 100, 1, 20, 3, 100 });

	EXPECT_EQ(hp_lost_to_the_ambush(), 4) << "surprised by a spider that was never hidden";
}

// Surprise lasts its round; the player is not left surprised for whatever acts next.
TEST_F(SpiderAmbushTest, TheSurpriseEndsWithItsRound)
{
	script({ 20, 4, 100, 1, 20, 3, 100 });

	spider.ai->update(spider, ctx);

	EXPECT_FALSE(player->has_state(ActorState::IS_SURPRISED));
}

// THAC0 20 against armour class -10 needs a 30, which no d20 reaches - nor a 29 in the
// surprise round.
TEST_F(SpiderAmbushTest, AnAmbushRollsAgainstArmourClass)
{
	player->armorClass = std::make_unique<ArmorClass>(-10);
	script({ 20, 4, 100, 1, 20, 4, 100 });

	EXPECT_EQ(hp_lost_to_the_ambush(), 0) << "the bite cannot hit this armour class";
}

// A protected player: the spider saves against the spell on a 12 and fails. The same
// 12 would hit armour class 10, so a spider that skipped the save would bite for four.
TEST_F(SpiderAmbushTest, SanctuaryHoldsOffASpiderThatFailsToSave)
{
	ASSERT_GT(SavingThrows::target(spider.get_creature_class(), spider.get_creature_level(), SavingThrow::SPELL), 12);
	player->add_state(ActorState::IS_PROTECTED);
	game.dice.set_next_roll(12);
	game.dice.set_next_roll(4);

	EXPECT_EQ(hp_lost_to_the_ambush(), 0) << "sanctuary held and the spider struck anyway";
}
