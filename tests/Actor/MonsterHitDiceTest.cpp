// file: MonsterHitDiceTest.cpp
// A monster's hit dice already stand for its constitution. AD&D 2e grants the
// constitution hit point adjustment to characters, never to monsters, so a
// creature built from hit dice must have the same hit points after its first
// turn as it had when it was made - and its tracker must know the score is
// accounted for, so a later drain is reported against the right value.
//
// The defect this pins: MonsterCreator recorded the score at creation and four
// other creation paths did not, so a spider, a mimic and a shopkeeper each took
// a constitution adjustment on top of their hit dice on turn one, silently.
//
// Expected values come from the dice forced below and from
// src/json/constitution.json read through the non-warrior cap: a score of 18
// would add +2 and a score of 6 would take 1 away, and neither may happen.

#include <gtest/gtest.h>

#include "src/Creature.h"
#include "src/Monsters.h"
#include "src/Spider.h"
#include "src/GameContext.h"
#include "src/Paths.h"
#include "src/ItemCreator.h"
#include "src/DataManager.h"
#include "src/LevelManager.h"
#include "src/ShopkeeperFactory.h"
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

	MockGameContext mock{};
	GameContext ctx{};
	DataManager dataManager{};
	LevelManager levelManager{};
};

// Web weaver: strength 3d6, dexterity 3d6, constitution 3d6, then d8+5 hit points.
TEST_F(MonsterHitDiceTest, AWebWeaverKeepsItsHitDice)
{
	force_rolls({ 3, 3, 3, 3, 3, 3, 6, 6, 6, 4 });
	Spider weaver{ Vector2D{ 0, 0 }, ctx, SpiderType::WEB_SPINNER };
	ASSERT_EQ(weaver.get_constitution(), 18);
	ASSERT_EQ(weaver.get_max_hp(), 9);

	weaver.update_constitution_bonus(ctx);

	EXPECT_EQ(weaver.get_max_hp(), 9) << "a monster took a constitution bonus on top of its hit dice";
	EXPECT_TRUE(weaver.get_last_constitution().has_value());
}

// Small spider: strength 3d6, dexterity 3d6, constitution 1d6, then d2+2 hit
// points. A score of 6 is a penalty, which must not be taken either.
TEST_F(MonsterHitDiceTest, ASmallSpiderKeepsItsHitDiceThroughAPenalty)
{
	force_rolls({ 3, 3, 3, 3, 3, 3, 6, 1 });
	Spider small{ Vector2D{ 0, 0 }, ctx, SpiderType::SMALL };
	ASSERT_EQ(small.get_constitution(), 6);
	ASSERT_EQ(small.get_max_hp(), 3);

	small.update_constitution_bonus(ctx);

	EXPECT_EQ(small.get_max_hp(), 3);
}

// Mimic: hit points d6+d4 first, then strength 3d6+2, dexterity 3d6,
// constitution 3d6, intelligence d4+2, wisdom 2d6+1, charisma d4.
TEST_F(MonsterHitDiceTest, AMimicKeepsItsHitDice)
{
	force_rolls({ 1, 1, 3, 3, 3, 3, 3, 3, 6, 6, 6, 1, 1, 1, 1 });
	Mimic mimic{ Vector2D{ 0, 0 }, ctx };
	ASSERT_EQ(mimic.get_constitution(), 18);
	ASSERT_EQ(mimic.get_max_hp(), 2);

	mimic.update_constitution_bonus(ctx);

	EXPECT_EQ(mimic.get_max_hp(), 2);
	EXPECT_TRUE(mimic.get_last_constitution().has_value());
}

// A shopkeeper never rolls a constitution, so there is no bonus to take - but
// the score must still be recorded, or the first drain would be reported
// against a value it never had.
TEST_F(MonsterHitDiceTest, AShopkeeperRecordsItsScoreAtCreation)
{
	const auto shopkeeper = ShopkeeperFactory::create_shopkeeper(Vector2D{ 0, 0 }, 1, ctx);
	ASSERT_TRUE(shopkeeper);

	EXPECT_TRUE(shopkeeper->get_last_constitution().has_value());
	shopkeeper->update_constitution_bonus(ctx);
	EXPECT_EQ(shopkeeper->get_max_hp(), 100);
}
