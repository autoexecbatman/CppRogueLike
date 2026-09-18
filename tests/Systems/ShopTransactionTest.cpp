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
		// A player built directly has strength 0, whose carry limit is 0, so every item
		// would be too heavy. Ten is the average 3d6 roll and gives a limit of 50.
		player.set_strength(10);
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

	// Puts one item on the shop's shelves and returns it, still owned by the shop.
	Item& shelve(const char* itemKey)
	{
		auto item = ItemCreator::create(itemKey, Vector2D{ 0, 0 }, mock.content_registry);
		EXPECT_TRUE(item);
		Item& held = *item;
		EXPECT_TRUE(InventoryOperations::add_item(shop.get_shop_inventory(), std::move(item)).has_value());
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

// A purchase hands over the item that was on the shelf - that object, moved, with its
// key and enhancement intact - and the shopkeeper receives the price.
TEST_F(ShopTransactionTest, APurchaseHandsOverTheShelvedItemAndPaysTheOwner)
{
	Item& onShelf = shelve("health_potion");
	const int price = shop.get_buy_price(onShelf);
	player.adjust_gold(price);
	const int buyerGoldBefore = player.get_gold();
	const std::size_t stockBefore = shop.get_shop_inventory().items.size();

	const bool bought = shop.process_player_purchase(ctx, onShelf, player, owner);

	ASSERT_TRUE(bought);
	ASSERT_EQ(player.inventoryData.items.size(), 1u) << "the price was paid and nothing was handed over";
	EXPECT_EQ(player.inventoryData.items.front().get(), &onShelf)
		<< "the pack must hold the shelved item itself, not a copy of some of its fields";
	EXPECT_EQ(shop.get_shop_inventory().items.size(), stockBefore - 1) << "the item left the shelf";
	EXPECT_EQ(player.get_gold(), buyerGoldBefore - price) << "the buyer paid";
	EXPECT_EQ(owner.get_gold(), price) << "the price went to the shopkeeper rather than vanishing";
}

// The key is what the damage registry reads, so a bought weapon without one hits for
// unarmed damage. The purchase used to copy the item field by field and leave it out.
TEST_F(ShopTransactionTest, ABoughtWeaponKeepsItsKey)
{
	Item& onShelf = shelve("long_sword");
	player.adjust_gold(shop.get_buy_price(onShelf));

	ASSERT_TRUE(shop.process_player_purchase(ctx, onShelf, player, owner));

	ASSERT_EQ(player.inventoryData.items.size(), 1u);
	EXPECT_EQ(player.inventoryData.items.front()->itemKey, "long_sword")
		<< "a weapon without its key is looked up as unarmed";
}

// A buyer who cannot pay is refused before anything moves, the shelf included.
TEST_F(ShopTransactionTest, AnUnaffordablePurchaseMovesNothing)
{
	Item& onShelf = shelve("long_sword");
	player.adjust_gold(-player.get_gold());
	ASSERT_GT(shop.get_buy_price(onShelf), 0) << "a free item cannot be unaffordable";
	const std::size_t stockBefore = shop.get_shop_inventory().items.size();

	const bool bought = shop.process_player_purchase(ctx, onShelf, player, owner);

	EXPECT_FALSE(bought);
	EXPECT_TRUE(player.inventoryData.items.empty());
	EXPECT_EQ(shop.get_shop_inventory().items.size(), stockBefore) << "the item stays on the shelf";
	EXPECT_EQ(owner.get_gold(), 0);
}

// Buying is held to the same strength-based weight limit as picking up. The purchase
// used the capacity-only add, so a shop would sell a player what they could not lift.
// Weights are set here rather than read from the data, so the case is about the rule
// and not about whichever item happens to be heaviest.
TEST_F(ShopTransactionTest, APurchaseTooHeavyToCarryMovesNothing)
{
	Item& onShelf = shelve("long_sword");
	onShelf.enhancement.weight = InventoryOperations::get_max_weight(player) + 1;
	player.adjust_gold(shop.get_buy_price(onShelf));
	const int buyerGoldBefore = player.get_gold();
	const std::size_t stockBefore = shop.get_shop_inventory().items.size();

	const bool bought = shop.process_player_purchase(ctx, onShelf, player, owner);

	EXPECT_FALSE(bought) << "a purchase over the carry limit went through";
	EXPECT_TRUE(player.inventoryData.items.empty());
	EXPECT_EQ(shop.get_shop_inventory().items.size(), stockBefore) << "the item stays on the shelf";
	EXPECT_EQ(player.get_gold(), buyerGoldBefore) << "nothing is paid for a purchase that did not happen";
}

// The boundary: exactly at the limit is carryable, one over is not.
TEST_F(ShopTransactionTest, APurchaseExactlyAtTheCarryLimitGoesThrough)
{
	Item& onShelf = shelve("long_sword");
	onShelf.enhancement.weight = InventoryOperations::get_max_weight(player);
	player.adjust_gold(shop.get_buy_price(onShelf));

	EXPECT_TRUE(shop.process_player_purchase(ctx, onShelf, player, owner))
		<< "an item that fits exactly must be sold";
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
