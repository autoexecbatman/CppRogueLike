// Checks that a shop stocks items, a purchase hands the item over, and a sale moves the
// item into the shop - in the build the game ships, not only the one tests usually run.
//
// What it is for. Every inventory add in the shop sat inside an assert:
// assert(add_item(...).has_value()). The shipped web build is Release, whose flags carry
// -DNDEBUG, and under NDEBUG an assert's argument is never evaluated - so the shop
// stocked nothing, a purchase took the gold and gave no item, and a sale took the item
// and put it nowhere. A Debug build runs the call, so a Debug suite passes either way;
// these cases only prove anything when run in Release.
//
// Run it in the configuration that ships:
//
//     cmake --build build --config Release --target test_exe
//     build\bin\Release\test_exe.exe --gtest_filter=ShopTransactionTest.*

#include <gtest/gtest.h>

#include <memory>

#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "src/ShopKeeper.h"
#include "tests/mocks/MockGameContext.h"

class ShopTransactionTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ItemCreator::load(Paths::ITEMS);
		ctx = mock.to_game_context();
	}

	MockGameContext mock{};
	GameContext ctx{};
	ShopKeeper shop{ ShopType::GENERAL_STORE, ShopQuality::AVERAGE };
	Player player{ Vector2D{ 1, 1 } };
};

TEST_F(ShopTransactionTest, AStockedShopHasSomethingToSell)
{
	// A potion shop draws one roll per item and nothing else, so three items cost the
	// count roll and three draws.
	ShopKeeper potionShop{ ShopType::POTION_SHOP, ShopQuality::AVERAGE };
	mock.dice.set_next_roll(3);
	for (int item = 0; item < 3; ++item)
	{
		mock.dice.set_next_roll(1);
	}

	potionShop.generate_initial_inventory(1, ctx);

	EXPECT_EQ(potionShop.get_shop_inventory().items.size(), 3u)
		<< "a shop that was stocked has nothing to sell";
}

TEST_F(ShopTransactionTest, APurchaseHandsTheItemOver)
{
	auto forSale = ItemCreator::create("health_potion", Vector2D{ 0, 0 }, mock.content_registry);
	ASSERT_TRUE(forSale);
	const int price = shop.get_buy_price(*forSale);
	player.adjust_gold(price);
	const int goldBefore = player.get_gold();
	const std::size_t carriedBefore = player.inventoryData.items.size();

	const bool bought = shop.process_player_purchase(ctx, *forSale, player);

	ASSERT_TRUE(bought);
	EXPECT_EQ(player.get_gold(), goldBefore - price) << "the price was paid";
	EXPECT_EQ(player.inventoryData.items.size(), carriedBefore + 1)
		<< "the price was paid and nothing was handed over";
}

TEST_F(ShopTransactionTest, ASaleMovesTheItemIntoTheShop)
{
	auto owned = ItemCreator::create("health_potion", Vector2D{ 0, 0 }, mock.content_registry);
	ASSERT_TRUE(owned);
	Item& toSell = *owned;
	ASSERT_TRUE(InventoryOperations::add_item(player.inventoryData, std::move(owned)).has_value());
	const std::size_t stockBefore = shop.get_shop_inventory().items.size();

	const bool sold = shop.process_player_sale(ctx, toSell, player);

	ASSERT_TRUE(sold);
	EXPECT_TRUE(player.inventoryData.items.empty()) << "the item left the pack";
	EXPECT_EQ(shop.get_shop_inventory().items.size(), stockBefore + 1)
		<< "the item left the pack and never reached the shop";
}
