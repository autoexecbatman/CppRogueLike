// file: MonsterHitDiceTest.cpp
// A monster's hit points are its hit dice with its Constitution on every one of
// them. The Dungeon Master Guide's Ability Scores for Monsters (PDF page 4860):
// a creature with ability scores "gains all the bonuses and penalties associated
// with its actual ability score as listed in the Player's Handbook", and
// "Constitution: All modifiers apply to creatures in the same way they do for
// characters. Monsters gain hit point bonuses for high scores as a warrior."
// This game rolls six scores for every monster, so every monster has them.
//
// Expected values are worked from src/json/constitution.json read as the warrior
// column - 18 adds 4 a die, 21 adds 6 and counts a roll under 3 as 3, 3 takes 2
// away - and from the Player's Handbook's floor (PDF page 32): "no Hit Die ever
// yields less than 1 hit point, regardless of modifications".

#include <gtest/gtest.h>

#include "src/Creature.h"
#include "src/DataManager.h"
#include "src/GameContext.h"
#include "src/ItemCreator.h"
#include "src/LevelManager.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/Monsters.h"
#include "src/Paths.h"
#include "src/ShopkeeperFactory.h"
#include "src/Spider.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// A score written as a fixed value rolls no dice, so a test's forced rolls are
// the hit dice alone.
DiceExpr fixed_score(int score)
{
	return DiceExpr{ 0, 0, score };
}
} // namespace

class MonsterHitDiceTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		dataManager.load_all_data(mock.messages);
		ctx = mock.to_game_context();
		ctx.dataManager = &dataManager;
		ctx.levelManager = &levelManager;
	}

	void force_rolls(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			mock.dice.set_next_roll(roll);
		}
	}

	// A monster whose six scores are fixed and whose hit dice are the caller's,
	// so the only dice rolled are the hit dice.
	MonsterParams params_with(int constitution, DiceExpr hitDice)
	{
		MonsterParams params = mock.monsterRegistry.get_params("goblin");
		params.equipment.clear();
		params.strDice = fixed_score(10);
		params.dexDice = fixed_score(10);
		params.conDice = fixed_score(constitution);
		params.intDice = fixed_score(10);
		params.wisDice = fixed_score(10);
		params.chaDice = fixed_score(10);
		params.hpDice = hitDice;
		return params;
	}

	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	LevelManager levelManager{};
};

// Constitution 18 on the warrior column adds 4 to each of three d8 rolled at 5.
// Capped at the non-warrior +2 the answer would be 21, and with no adjustment 15.
TEST_F(MonsterHitDiceTest, AMonsterAddsItsConstitutionToEveryHitDie)
{
	const MonsterParams params = params_with(18, DiceExpr{ 3, 8, 0 });
	force_rolls({ 5, 5, 5 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	ASSERT_EQ(monster->get_constitution(), 18);
	EXPECT_EQ(monster->get_max_hp(), 27);
}

// The bonus written into a hit dice expression - the giant snake's 4d8+4 - is
// the monster's, not a die, so the adjustment does not ride on it. Two d8 at 5
// with Constitution 18 are 9 apiece, and the 4 is added once.
TEST_F(MonsterHitDiceTest, TheFlatBonusOnHitDiceTakesNoAdjustment)
{
	const MonsterParams params = params_with(18, DiceExpr{ 2, 8, 4 });
	force_rolls({ 5, 5 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_max_hp(), 22);
}

// Constitution 3 takes 2 off each die, and the two low rolls floor at 1 rather
// than at 0 and -1. Without the floor the three dice would come to 2.
TEST_F(MonsterHitDiceTest, NoHitDieOfAMonsterYieldsLessThanOne)
{
	const MonsterParams params = params_with(3, DiceExpr{ 3, 8, 0 });
	force_rolls({ 1, 2, 5 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	ASSERT_EQ(monster->get_constitution(), 3);
	EXPECT_EQ(monster->get_max_hp(), 5);
}

// Table 3's footnote for a score of 21: all 1s and 2s rolled on hit dice are
// considered 3s, and the bonus is 6. A rolled 1 is worth 9 and a rolled 8 is
// worth 14. Without the footnote the pair would come to 21.
TEST_F(MonsterHitDiceTest, AMonsterOfTwentyOneCountsALowRollAsThree)
{
	const MonsterParams params = params_with(21, DiceExpr{ 2, 8, 0 });
	force_rolls({ 1, 8 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_max_hp(), 23);
}

// The adjustment is applied once, when the creature is made, so the first round
// of upkeep finds the score already accounted for and moves nothing.
TEST_F(MonsterHitDiceTest, AMonsterKeepsWhatItWasMadeWith)
{
	const MonsterParams params = params_with(18, DiceExpr{ 3, 8, 0 });
	force_rolls({ 5, 5, 5 });
	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);
	ASSERT_EQ(monster->get_max_hp(), 27);

	monster->update_constitution_bonus(ctx);

	EXPECT_EQ(monster->get_max_hp(), 27) << "the score was counted twice";
	EXPECT_TRUE(monster->get_last_constitution().has_value());
}

// A score that moves later moves the hit points by the difference on every hit
// die, which for a monster is every die it has. 14 to 18 is 0 to +4 a die, and
// three hit dice carry it three times.
TEST_F(MonsterHitDiceTest, AMonstersChangedScoreMovesByItsHitDice)
{
	Creature monster{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "pit fiend", 0 } };
	monster.set_creature_level(3);
	monster.set_constitution(14);
	monster.set_hit_dice(30);
	monster.set_constitution(18);

	monster.update_constitution_bonus(ctx);

	EXPECT_EQ(monster.get_max_hp(), 42);
}

// Web weaver: strength 3d6, dexterity 3d6, constitution 3d6, then d8+5 hit
// points. A score of 18 adds 4 to the die and leaves the 5 alone.
TEST_F(MonsterHitDiceTest, AWebWeaverTakesItsConstitutionOnItsHitDie)
{
	force_rolls({ 3, 3, 3, 3, 3, 3, 6, 6, 6, 4 });
	Spider weaver{ Vector2D{ 0, 0 }, ctx, SpiderType::WEB_SPINNER };

	ASSERT_EQ(weaver.get_constitution(), 18);
	EXPECT_EQ(weaver.get_max_hp(), 13);
}

// Small spider: strength 3d6, dexterity 3d6, constitution 1d6, then d2+2 hit
// points. A score of 6 takes 1 off the die, and a rolled 2 is worth 1.
TEST_F(MonsterHitDiceTest, ASmallSpiderTakesItsConstitutionPenalty)
{
	force_rolls({ 3, 3, 3, 3, 3, 3, 6, 2 });
	Spider small{ Vector2D{ 0, 0 }, ctx, SpiderType::SMALL };

	ASSERT_EQ(small.get_constitution(), 6);
	EXPECT_EQ(small.get_max_hp(), 3);
}

// Mimic: strength 3d6+2, dexterity 3d6, constitution 3d6, intelligence d4+2,
// wisdom 2d6+1, charisma d4, then a d6 and a d4 of hit points. Two dice, so a
// score of 18 adds 4 twice.
TEST_F(MonsterHitDiceTest, AMimicTakesItsConstitutionOnBothHitDice)
{
	force_rolls({ 3, 3, 3, 3, 3, 3, 6, 6, 6, 1, 1, 1, 1, 1, 1 });
	Mimic mimic{ Vector2D{ 0, 0 }, ctx };

	ASSERT_EQ(mimic.get_constitution(), 18);
	EXPECT_EQ(mimic.get_max_hp(), 10);

	// The score reached the dice, so it is spent and the first round adds none of
	// it again - which a pool built without set_hit_dice would.
	mimic.update_constitution_bonus(ctx);

	EXPECT_EQ(mimic.get_max_hp(), 10);
	EXPECT_TRUE(mimic.get_last_constitution().has_value());
}

// A shopkeeper is given a hit point total rather than hit dice, and a total has
// no dice for a score to adjust - the score is still recorded as accounted for.
TEST_F(MonsterHitDiceTest, AShopkeeperRecordsItsScoreAtCreation)
{
	const auto shopkeeper = ShopkeeperFactory::create_shopkeeper(Vector2D{ 0, 0 }, 1, ctx);
	ASSERT_TRUE(shopkeeper);

	EXPECT_TRUE(shopkeeper->get_last_constitution().has_value());
	shopkeeper->update_constitution_bonus(ctx);
	EXPECT_EQ(shopkeeper->get_max_hp(), 100);
}
