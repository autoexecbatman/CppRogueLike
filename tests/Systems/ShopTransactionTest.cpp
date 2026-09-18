// Checks that a shop stocks items, a purchase hands the item over, and a sale puts the
// item on the shelves the buy menu reads - with the shopkeeper paying, and nothing moving
// when the sale is refused.
//
// What it is for. Two defects, one per half of the shop.
//
// Stocking and buying sat inside an assert: assert(add_item(...).has_value()). The
// shipped web build is Release, whose flags carry -DNDEBUG, and under NDEBUG an
// assert's argument is never evaluated - so the shop stocked nothing and a purchase
// took the gold and gave no item. A Debug build runs the call, so those two cases only
// prove anything when this file is built in Release.
//
// Selling had its own code in MenuSell, which put a sold item into the shopkeeper
// creature's pack - a store the buy menu never reads - and, when that pack was full,
// re-added an item it had already destroyed. It now goes through
// ShopKeeper::process_player_sale, the one place a sale is decided.
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
	// Puts one item into the seller's pack and returns it, still owned by the pack.
	Item& give_seller(const char* itemKey)
	{
		auto item = ItemCreator::create(itemKey, Vector2D{ 0, 0 }, mock.content_registry);
		EXPECT_TRUE(item);
		Item& held = *item;
		EXPECT_TRUE(InventoryOperations::add_item(player.inventoryData, std::move(item)).has_value());
		return held;
	}

	// Stocks the shop to its capacity, so nothing more fits on the shelves.
	void fill_shelves()
	{
		FloorInventory& shelves = shop.get_shop_inventory();
		while (!InventoryOperations::is_inventory_full(shelves))
		{
			ASSERT_TRUE(InventoryOperations::add_item(
				shelves,
				ItemCreator::create("health_potion", Vector2D{ 0, 0 }, mock.content_registry)).has_value());
		}
	}

	ShopKeeper shop{ ShopType::GENERAL_STORE, ShopQuality::AVERAGE };
	Player player{ Vector2D{ 1, 1 } };
	// The shopkeeper creature: the shop's goods live in the ShopKeeper, its gold here.
	Creature owner{ Vector2D{ 2, 1 }, ActorData{ TileRef{}, "shopkeeper", 0 } };
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

// A sale moves the item onto the shelves the buy menu reads, and the shopkeeper - the
// only creature in the trade that holds gold - pays for it.
TEST_F(ShopTransactionTest, ASaleShelvesTheItemAndTheOwnerPays)
{
	Item& toSell = give_seller("health_potion");
	const int price = shop.get_sell_price(toSell);
	owner.adjust_gold(price);
	const int sellerGoldBefore = player.get_gold();
	const std::size_t stockBefore = shop.get_shop_inventory().items.size();

	const bool sold = shop.process_player_sale(ctx, toSell, player, owner);

	ASSERT_TRUE(sold);
	EXPECT_TRUE(player.inventoryData.items.empty()) << "the item left the pack";
	EXPECT_EQ(shop.get_shop_inventory().items.size(), stockBefore + 1)
		<< "the item must reach the shelves the buy menu reads";
	EXPECT_EQ(player.get_gold(), sellerGoldBefore + price) << "the seller was paid";
	EXPECT_EQ(owner.get_gold(), 0) << "the shopkeeper paid, rather than the gold appearing from nowhere";
}

// A shop with no room refuses before anything moves. It used to pay, take the item out
// of the pack, fail to shelve it, and destroy it.
TEST_F(ShopTransactionTest, ASaleToFullShelvesMovesNothing)
{
	fill_shelves();
	Item& toSell = give_seller("health_potion");
	owner.adjust_gold(shop.get_sell_price(toSell));
	const int sellerGoldBefore = player.get_gold();
	const int ownerGoldBefore = owner.get_gold();

	const bool sold = shop.process_player_sale(ctx, toSell, player, owner);

	EXPECT_FALSE(sold);
	EXPECT_EQ(player.inventoryData.items.size(), 1u) << "the item stays in the pack";
	EXPECT_EQ(player.get_gold(), sellerGoldBefore) << "nothing is paid for a sale that did not happen";
	EXPECT_EQ(owner.get_gold(), ownerGoldBefore);
}

// A shopkeeper who cannot pay refuses before anything moves.
TEST_F(ShopTransactionTest, ASaleTheOwnerCannotAffordMovesNothing)
{
	Item& toSell = give_seller("health_potion");
	ASSERT_GT(shop.get_sell_price(toSell), 0) << "a free item cannot be unaffordable";
	ASSERT_EQ(owner.get_gold(), 0);
	const int sellerGoldBefore = player.get_gold();
	const std::size_t stockBefore = shop.get_shop_inventory().items.size();

	const bool sold = shop.process_player_sale(ctx, toSell, player, owner);

	EXPECT_FALSE(sold);
	EXPECT_EQ(player.inventoryData.items.size(), 1u) << "the item stays in the pack";
	EXPECT_EQ(shop.get_shop_inventory().items.size(), stockBefore);
	EXPECT_EQ(player.get_gold(), sellerGoldBefore);
}
