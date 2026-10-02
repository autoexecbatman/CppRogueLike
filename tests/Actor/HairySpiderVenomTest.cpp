// file: HairySpiderVenomTest.cpp
// The small spider's venom, by the book.
//
// What it is for. The game's three spiders are invented, so each is matched to the
// Monstrous Manual spider it is closest to (PDF pages 1876-1879 of the 2e archive).
// The small spider is the Manual's hairy spider - a hunting spider, THAC0 20, the
// lowest experience of the three - whose poison is not in Table 51 at all. Its entry
// prints the whole rule: "Victims receive a +2 bonus to saving throws vs. the hairy
// spiders' weak poison. If the saving throw fails, the victim's AC and attack rolls
// are penalized by 1, and Dexterity is penalized by -3 with respect to Dexterity
// checks. These effects begin one round after the bite and last for 1d4+1 rounds."
//
// So this venom does no damage at all, which is what the 1d3 points it used to take
// got wrong. It penalises, it is resisted at +2, it lands a round late, and it wears
// off. The Dexterity penalty has nothing to attach to - the game has no Dexterity
// check separate from the score - and is left out.
//
// Poison is injected "by bite or sting" (PDF page 736), so a bite that never landed
// injects nothing. The saving throw is the victim's against paralyzation, poison and
// death magic, with the Constitution adjustment its own table prints and the +2 the
// spider's entry grants on top.
//
// Every roll is scripted in the order the game asks for it: the attack roll, the
// damage die, the save against the venom, then the 1d4 that sets how long it lasts.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=HairySpiderVenomTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>

#include "src/Actor.h"
#include "src/AiSpider.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/DamageInfo.h"
#include "src/DataManager.h"
#include "src/ExperienceReward.h"
#include "src/Game.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/MessageSystem.h"
#include "src/MonsterAttacker.h"
#include "src/Player.h"
#include "src/SavingThrow.h"
#include "src/TurnSchedule.h"

class HairySpiderVenomTest : public ::testing::Test
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

		// A 1d4 bite from THAC0 20, which needs a 10 against armour class 10.
		spider.experienceReward = std::make_unique<ExperienceReward>(0);
		spider.healthPool = std::make_unique<HealthPool>(10);
		spider.armorClass = std::make_unique<ArmorClass>(7);
		spider.attacker = std::make_unique<MonsterAttacker>(spider, DamageInfo{ "1d4", DamageType::PHYSICAL });
		spider.set_dr(0);
		spider.set_thaco(20);
		spider.set_strength(8);
		spider.set_dexterity(10);
		spider.set_natural_attack("fangs");
		spider.ai = std::make_unique<AiSpider>();
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

	// The roll this player needs against the venom: its own target, less the
	// Constitution adjustment, less the +2 the hairy spider's entry grants.
	int venom_save_target()
	{
		const int constitutionBonus = game.dataManager.constitution_for(player->get_constitution()).PoisonSave;
		const int ownTarget = SavingThrows::target(player->get_creature_class(), player->get_creature_level(), SavingThrow::PARALYZATION_POISON_DEATH);
		return ownTarget - constitutionBonus - SPIDERS_OWN_SAVE_BONUS;
	}

	void one_turn()
	{
		spider.ai->update(spider, ctx);
	}

	// Rounds pass for the player alone, which is what carries the onset and the wearing
	// off. The clock moves with them: a duration is a reading on it, so asking the buff
	// system twice at one reading is asking the same question twice.
	void rounds_pass(int count)
	{
		for (int round = 0; round < count; ++round)
		{
			ctx.gameState->advance_clock(TIME_UNITS_PER_ROUND);
			player->tick_poison(ctx);
			ctx.buffSystem->update_creature_buffs(*player, ctx.gameState->get_time());
		}
	}

	// Every part of every finalized message, joined. A venom that takes no hit
	// points leaves the log as the only place the player sees it at all.
	std::string all_message_text() const
	{
		std::string joined;
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			for (const auto& part : ctx.messageSystem->get_attack_message_at(index))
			{
				joined += part.text;
			}
		}
		return joined;
	}

	static constexpr int STARTING_HP = 100;
	static constexpr int BITE_DIE = 4;
	static constexpr int HIT_ROLL = 10;
	// "Victims receive a +2 bonus to saving throws vs. the hairy spiders' weak poison."
	static constexpr int SPIDERS_OWN_SAVE_BONUS = 2;

	Game game;
	GameContext ctx;
	Map map{ 20, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 4, 5 }) };
	Creature spider{ Vector2D{ 5, 5 }, ActorData{ TileRef{}, "small spider", ColorPairId::WHITE_BLACK } };
};

// "the victim's AC and attack rolls are penalized by 1... These effects begin one
// round after the bite and last for 1d4+1 rounds."
TEST_F(HairySpiderVenomTest, AFailedSaveCostsOnePointOfArmourClassAndOneToHit)
{
	// The buff system's contributions are what the venom moves; carrying them into
	// ArmorClass is a refresh of its own that a turn runs.
	ASSERT_EQ(ctx.buffSystem->calculate_ac_bonus(*player), 0);
	ASSERT_EQ(ctx.buffSystem->calculate_hit_modifier(*player), 0);

	// The bite lands for one, the save fails by a point, and the 1d4 shows 2, so the
	// penalty runs for three rounds.
	script({ HIT_ROLL, 1, venom_save_target() - 1, 2 });
	one_turn();

	// It begins one round after the bite, so nothing has changed yet.
	EXPECT_EQ(ctx.buffSystem->calculate_ac_bonus(*player), 0) << "the venom has not taken hold";

	rounds_pass(1);
	EXPECT_EQ(ctx.buffSystem->calculate_ac_bonus(*player), 1) << "one point of armour class worse";
	EXPECT_EQ(ctx.buffSystem->calculate_hit_modifier(*player), -1) << "and one worse to hit";
	EXPECT_TRUE(ctx.buffSystem->has_buff(*player, BuffType::HAIRY_SPIDER_VENOM));
}

// "weak poison": it takes no hit points at all, where the invented venom took 1d3.
TEST_F(HairySpiderVenomTest, TheVenomTakesNoHitPoints)
{
	script({ HIT_ROLL, 1, venom_save_target() - 1, 2 });
	one_turn();
	const int afterBite = player->get_hp();

	rounds_pass(4);

	EXPECT_EQ(player->get_hp(), afterBite) << "the bite's damage is all it costs";
}

// "last for 1d4+1 rounds": the venom carries that many when it lands. How many ticks
// of which system spend them is the buff system's business - the book only says how
// long, so that is what this asks.
TEST_F(HairySpiderVenomTest, TheVenomLastsOneDFourPlusOneRounds)
{
	// The 1d4 shows two, so three rounds.
	script({ HIT_ROLL, 1, venom_save_target() - 1, 2 });
	one_turn();
	rounds_pass(1);
	ASSERT_TRUE(ctx.buffSystem->has_buff(*player, BuffType::HAIRY_SPIDER_VENOM));

	EXPECT_EQ(ctx.buffSystem->get_buff_turns(*player, BuffType::HAIRY_SPIDER_VENOM, ctx.gameState->get_time()), 2 + 1)
		<< "1d4 of two, plus one, and the round it lands in is not spent landing";
}

// And it does run out, taking its point with it.
TEST_F(HairySpiderVenomTest, ThePenaltyGoesWhenTheVenomDoes)
{
	// The largest 1d4 the book allows, so five rounds; ten is past any of them.
	script({ HIT_ROLL, 1, venom_save_target() - 1, 4 });
	one_turn();
	rounds_pass(1);
	ASSERT_TRUE(ctx.buffSystem->has_buff(*player, BuffType::HAIRY_SPIDER_VENOM));

	rounds_pass(10);

	EXPECT_FALSE(ctx.buffSystem->has_buff(*player, BuffType::HAIRY_SPIDER_VENOM));
	EXPECT_EQ(ctx.buffSystem->calculate_ac_bonus(*player), 0) << "and the point comes back";
	EXPECT_EQ(ctx.buffSystem->calculate_hit_modifier(*player), 0);
}

// "Victims receive a +2 bonus to saving throws vs. the hairy spiders' weak poison."
//
// This is the case the bonus decides. The roll is two under what the victim would
// otherwise need, so without the spider's +2 it fails and the venom takes hold; with
// it, the save is made and nothing does.
TEST_F(HairySpiderVenomTest, TheSaveTakesTheSpidersOwnTwoPointBonus)
{
	script({ HIT_ROLL, 1, venom_save_target() });
	one_turn();
	rounds_pass(1);

	EXPECT_FALSE(ctx.buffSystem->has_buff(*player, BuffType::HAIRY_SPIDER_VENOM));

	// One under that roll is a failure even with the bonus, which is what says the
	// boundary is where the book puts it.
	script({ HIT_ROLL, 1, venom_save_target() - 1, 2 });
	one_turn();
	rounds_pass(1);

	EXPECT_TRUE(ctx.buffSystem->has_buff(*player, BuffType::HAIRY_SPIDER_VENOM));
}

// Poison is injected by a bite, so a bite that missed injects nothing. No save and no
// duration are scripted: a venom that ran anyway would reach for a real roll.
TEST_F(HairySpiderVenomTest, ABiteThatMissesInjectsNothing)
{
	script({ 1 });
	one_turn();
	rounds_pass(2);

	EXPECT_FALSE(ctx.buffSystem->has_buff(*player, BuffType::HAIRY_SPIDER_VENOM));
	EXPECT_EQ(player->get_hp(), STARTING_HP);
}

// The bite is the only word the player gets before the onset, and the spider that
// bit is what the message names - the sibling venoms name the biter too.
TEST_F(HairySpiderVenomTest, TheBiteSaysWhetherTheVenomTookHold)
{
	script({ HIT_ROLL, 1, venom_save_target() - 1, 2 });
	one_turn();

	EXPECT_NE(all_message_text().find("small spider"), std::string::npos) << "the biter is named";
	EXPECT_NE(all_message_text().find("will take hold"), std::string::npos);

	// And a save that is made says so, rather than leaving the bite unexplained.
	script({ HIT_ROLL, 1, venom_save_target() });
	one_turn();

	EXPECT_NE(all_message_text().find("does not take hold"), std::string::npos);
}

// "These effects begin one round after the bite": the round it lands in is the
// round the penalty appears, so that is the round it has to be announced.
TEST_F(HairySpiderVenomTest, TheOnsetIsAnnouncedWhenItArrives)
{
	script({ HIT_ROLL, 1, venom_save_target() - 1, 2 });
	one_turn();
	ASSERT_EQ(all_message_text().find("weakened by the venom"), std::string::npos)
		<< "nothing has taken hold yet";

	rounds_pass(1);

	EXPECT_NE(all_message_text().find("weakened by the venom"), std::string::npos);
}

// end of file: HairySpiderVenomTest.cpp
