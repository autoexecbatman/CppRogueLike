// file: ShopStockLevelTest.cpp
// A shop stocks for the dungeon level it was placed at, and it is told that
// level: it does not read a level from the context that may say something
// else. The defect this pins had configure_shopkeeper choosing shop type and
// quality from its dungeonLevel parameter while the stock generator read
// ctx.levelManager - one fact from two sources.
//
// The observable is armour, the category with real level gates in
// data/content/items.json. Candidates are walked in key order and a roll past
// the total weight returns the last one, so forcing the roll high makes the
// draw deterministic: splint_mail (floor 5) at level 20, studded_leather
// (floor 1) at level 1. Expected keys come from that data, not from the code.

#include <gtest/gtest.h>

#include "src/Item.h"
#include "src/GameContext.h"
#include "src/Paths.h"
#include "src/ItemCreator.h"
#include "src/LevelManager.h"
#include "src/ShopKeeper.h"
#include "tests/mocks/MockGameContext.h"

class ShopStockLevelTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.levelManager = &levelManager;
	}

	// Three items, each the last armour candidate, none enhanced: the count roll,
	// then per item a draw past any total weight and an enhancement roll of 100.
	void force_three_last_candidates()
	{
		mock.dice.set_next_roll(3);
		for (int item = 0; item < 3; ++item)
		{
			mock.dice.set_next_roll(10000);
			mock.dice.set_next_roll(100);
		}
	}

	std::vector<std::string> stocked_keys(const ShopKeeper& shop) const
	{
		std::vector<std::string> keys;
		for (const auto& item : shop.get_shop_inventory().items)
		{
			keys.push_back(item->itemKey);
		}
		return keys;
	}

	MockGameContext mock{};
	GameContext ctx{};
	LevelManager levelManager{};
};

// Told level 20 while the context's manager still says 1, the shop stocks for 20.
TEST_F(ShopStockLevelTest, StocksForTheLevelItIsTold)
{
	ASSERT_EQ(levelManager.get_dungeon_level(), 1) << "the manager must disagree with the level the shop is told";
	ShopKeeper shop{ ShopType::ARMOR_SHOP, ShopQuality::AVERAGE };
	force_three_last_candidates();

	shop.generate_initial_inventory(20, ctx);

	EXPECT_EQ(stocked_keys(shop), (std::vector<std::string>{ "splint_mail", "splint_mail", "splint_mail" }))
		<< "the stock was drawn at the manager's level, not the one the shop was told";
}

// Told level 1, it stocks for 1, whatever else the context holds.
TEST_F(ShopStockLevelTest, StocksForLevelOneWhenToldOne)
{
	ShopKeeper shop{ ShopType::ARMOR_SHOP, ShopQuality::AVERAGE };
	force_three_last_candidates();

	shop.generate_initial_inventory(1, ctx);

	EXPECT_EQ(stocked_keys(shop), (std::vector<std::string>{ "studded_leather", "studded_leather", "studded_leather" }));
}

// Being told the level is the whole of it: no level manager is consulted.
TEST_F(ShopStockLevelTest, NeedsNoLevelManager)
{
	ctx.levelManager = nullptr;
	ShopKeeper shop{ ShopType::ARMOR_SHOP, ShopQuality::AVERAGE };
	force_three_last_candidates();

	shop.generate_initial_inventory(20, ctx);

	EXPECT_EQ(stocked_keys(shop).size(), 3u);
}

// The level is used exactly, not nearly. Computed from the data: the last armour
// candidate is studded_leather when told 5 and splint_mail when told 6, so an
// off-by-one in either direction lands on the other side of that line.
TEST_F(ShopStockLevelTest, ToldBelowTheBoundaryStocksTheLowerCandidate)
{
	ShopKeeper shop{ ShopType::ARMOR_SHOP, ShopQuality::AVERAGE };
	force_three_last_candidates();

	shop.generate_initial_inventory(5, ctx);

	EXPECT_EQ(stocked_keys(shop).front(), "studded_leather");
}

TEST_F(ShopStockLevelTest, ToldAtTheBoundaryStocksTheHigherCandidate)
{
	ShopKeeper shop{ ShopType::ARMOR_SHOP, ShopQuality::AVERAGE };
	force_three_last_candidates();

	shop.generate_initial_inventory(6, ctx);

	EXPECT_EQ(stocked_keys(shop).front(), "splint_mail");
}
