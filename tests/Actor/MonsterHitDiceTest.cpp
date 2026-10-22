// file: MonsterHitDiceTest.cpp
// A monster's hit points are its hit dice with its Constitution on every one of
// them. The Dungeon Master Guide's Ability Scores for Monsters (PDF page 4860):
// a creature with ability scores "gains all the bonuses and penalties associated
// with its actual ability score as listed in the Player's Handbook", and
// "Constitution: All modifiers apply to creatures in the same way they do for
// characters. Monsters gain hit point bonuses for high scores as a warrior."
// This game rolls six scores for every monster, so every monster has them, and reads
// its Constitution from Table 8: 3d6 on the column for its size (PDF page 4858).
//
// Expected values are worked from src/json/constitution.json read as the warrior
// column - 18 adds 4 a die, 21 adds 6 and counts a roll under 3 as 3, 3 takes 2
// away - and from the Player's Handbook's floor (PDF page 32): "no Hit Die ever
// yields less than 1 hit point, regardless of modifications".

#include <gtest/gtest.h>

#include <memory>
#include <string_view>

#include "src/Colors.h"
#include "src/Creature.h"
#include "src/CreatureSize.h"
#include "src/DataManager.h"
#include "src/GameContext.h"
#include "src/ItemCreator.h"
#include "src/LevelManager.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"
#include "src/RandomDice.h"
#include "tests/mocks/MockGameContext.h"

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

	// Queues a data-built monster's dice in the order it rolls them: Dexterity's 3d6, then
	// the Constitution the test names, then Intelligence, Wisdom and Charisma's 3d6 each,
	// then the hit dice the test names. The scores the test does not name roll 3s.
	void force_constitution_and_hit_dice(std::initializer_list<int> constitution, std::initializer_list<int> hitDice)
	{
		force_rolls({ 3, 3, 3 });
		force_rolls(constitution);
		force_rolls({ 3, 3, 3, 3, 3, 3, 3, 3, 3 });
		force_rolls(hitDice);
	}

	// A monster of the caller's size and hit dice with a printed Strength, so it rolls no
	// Strength die; its other scores come from the tables.
	// A monster built from its record in monsters.json, as the spawn table builds one.
	std::unique_ptr<Creature> build(std::string_view key)
	{
		return MonsterCreator::create_from_params(Vector2D{ 0, 0 }, mock.monsterRegistry.get_params(key), ctx);
	}

	MonsterParams params_with(CreatureSize size, DiceExpr hitDice)
	{
		MonsterParams params = mock.monsterRegistry.get_params("goblin");
		params.equipment.clear();
		params.strength = MonsterStrength{ 10, 0 };
		params.size = size;
		params.hitDice = MonsterHitDice{
			hitDice.sides,
			hitDice.bonus,
			{ { hitDice.num, 0 } },
		};
		return params;
	}

	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	LevelManager levelManager{};
};

// A man-sized monster rolling 17 reads Constitution 18, which on the warrior column
// adds 4 to each of three d8 rolled at 5. Capped at the non-warrior +2 the answer
// would be 21, and with no adjustment 15.
TEST_F(MonsterHitDiceTest, AMonsterAddsItsConstitutionToEveryHitDie)
{
	const MonsterParams params = params_with(CreatureSize::MEDIUM, DiceExpr{ 3, 8, 0 });
	force_constitution_and_hit_dice({ 6, 6, 5 }, { 5, 5, 5 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	ASSERT_EQ(monster->get_constitution(), 18);
	EXPECT_EQ(monster->get_max_hp(), 27);
}

// The bonus written into a hit dice expression - the giant snake's 4d8+4 - is
// the monster's, not a die, so the adjustment does not ride on it. Two d8 at 5
// with Constitution 18 are 9 apiece, and the 4 is added once.
TEST_F(MonsterHitDiceTest, TheFlatBonusOnHitDiceTakesNoAdjustment)
{
	const MonsterParams params = params_with(CreatureSize::MEDIUM, DiceExpr{ 2, 8, 4 });
	force_constitution_and_hit_dice({ 6, 6, 5 }, { 5, 5 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_max_hp(), 22);
}

// A tiny monster rolling 3 reads Constitution 3, the table's lowest, which takes 2
// off each die, and the two low rolls floor at 1 rather
// than at 0 and -1. Without the floor the three dice would come to 2.
TEST_F(MonsterHitDiceTest, NoHitDieOfAMonsterYieldsLessThanOne)
{
	const MonsterParams params = params_with(CreatureSize::TINY, DiceExpr{ 3, 8, 0 });
	force_constitution_and_hit_dice({ 1, 1, 1 }, { 1, 2, 5 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	ASSERT_EQ(monster->get_constitution(), 3);
	EXPECT_EQ(monster->get_max_hp(), 5);
}

// A large monster rolling 18 reads Constitution 21. Table 3's footnote for that
// score: all 1s and 2s rolled on hit dice are
// considered 3s, and the bonus is 6. A rolled 1 is worth 9 and a rolled 8 is
// worth 14. Without the footnote the pair would come to 21.
TEST_F(MonsterHitDiceTest, AMonsterOfTwentyOneCountsALowRollAsThree)
{
	const MonsterParams params = params_with(CreatureSize::LARGE, DiceExpr{ 2, 8, 0 });
	force_constitution_and_hit_dice({ 6, 6, 6 }, { 1, 8 });

	const auto monster = MonsterCreator::create_from_params(Vector2D{ 0, 0 }, params, ctx);

	EXPECT_EQ(monster->get_max_hp(), 23);
}

// The adjustment is applied once, when the creature is made, so the first round
// of upkeep finds the score already accounted for and moves nothing.
TEST_F(MonsterHitDiceTest, AMonsterKeepsWhatItWasMadeWith)
{
	const MonsterParams params = params_with(CreatureSize::MEDIUM, DiceExpr{ 3, 8, 0 });
	force_constitution_and_hit_dice({ 6, 6, 5 }, { 5, 5, 5 });
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
	Creature monster{
		Vector2D{ 0, 0 },
		ActorData{ TileRef{}, "pit fiend", ColorPairId::WHITE_BLACK },
	};
	monster.set_creature_level(3);
	monster.set_constitution(14);
	monster.set_hit_dice(30);
	monster.set_constitution(18);

	monster.update_constitution_bonus(ctx);

	EXPECT_EQ(monster.get_max_hp(), 42);
}

// Web weaver, the book's giant spider at 3+3 and large: strength 3d6, dexterity 3d6,
// then Constitution 3d6 read from Table 8, a 15 reading 18, then intelligence, wisdom
// and charisma 3d6 each, then three d8. A score of 18 adds 4 to each die, and the 3 is
// added once to the total: 3 x (4 + 4) + 3.
TEST_F(MonsterHitDiceTest, AWebWeaverTakesItsConstitutionOnEachHitDie)
{
	force_rolls({ 3, 3, 3, 3, 3, 3, 5, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4 });
	const auto weaver = build("spider_weaver");

	ASSERT_EQ(weaver->get_constitution(), 18);
	EXPECT_EQ(weaver->get_max_hp(), 27);
}

// Small spider, the book's hairy spider at 1-1 and tiny: strength 3d6, dexterity 3d6,
// then Constitution 3d6 read from Table 8, a 6 reading 6, then intelligence, wisdom and
// charisma 3d6 each, then one d8. A score of 6 takes 1 off the die and the -1 is taken
// once from the total: 5 - 1 - 1.
TEST_F(MonsterHitDiceTest, ASmallSpiderTakesItsConstitutionPenalty)
{
	force_rolls({ 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 5 });
	const auto small = build("spider_small");

	ASSERT_EQ(small->get_constitution(), 6);
	EXPECT_EQ(small->get_max_hp(), 3);
}

// Mimic, the killer mimic and large: strength 3d6, dexterity 3d6, Constitution 3d6
// read from Table 8 (a 15 reading 18), intelligence, wisdom and charisma 3d6 each,
// then 9 or 10 hit dice and a d8 for each. A score of 18 adds 4 to every one of them.
TEST_F(MonsterHitDiceTest, AMimicTakesItsConstitutionOnEveryHitDie)
{
	force_rolls({
		3,
		3,
		3,
		3,
		3,
		3,
		5,
		5,
		5,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		9,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
	});
	const auto mimic = build("mimic");

	ASSERT_EQ(mimic->get_constitution(), 18);
	EXPECT_EQ(mimic->get_max_hp(), 45);

	// The score reached the dice, so it is spent and the first round adds none of
	// it again - which a pool built without set_hit_dice would.
	mimic->update_constitution_bonus(ctx);

	EXPECT_EQ(mimic->get_max_hp(), 45);
	EXPECT_TRUE(mimic->get_last_constitution().has_value());
}

// The shopkeeper's 1d1+99 is one die, so its Constitution rides on it like any monster's:
// every die at its highest gives 19 on Table 8, man-sized, worth 5, so 1 + 5 + 99. The
// score is spent at creation and the first round adds none of it again.
TEST_F(MonsterHitDiceTest, AShopkeeperTakesItsConstitutionOnItsOneDie)
{
	// Building it stocks a shop, which rolls far more than the monster does.
	for (int roll = 0; roll < 400; ++roll)
	{
		mock.dice.set_next_roll(RandomDice::HIGHEST);
	}
	const auto shopkeeper = build("shopkeeper");

	ASSERT_EQ(shopkeeper->get_constitution(), 19);
	EXPECT_EQ(shopkeeper->get_max_hp(), 105);

	shopkeeper->update_constitution_bonus(ctx);

	EXPECT_EQ(shopkeeper->get_max_hp(), 105);
	EXPECT_TRUE(shopkeeper->get_last_constitution().has_value());
}
