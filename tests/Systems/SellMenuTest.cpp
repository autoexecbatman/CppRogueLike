// file: SellMenuTest.cpp
// The sell menu, driven the way a player drives it: opened on a shopkeeper with the
// player's pack, and a key pressed. ShopTransactionTest covers the shop's own sale;
// this covers the menu that calls it, which no test could build until a renderer
// existed, because MenuSell sizes itself from the viewport and its box from the font.
//
// Each test runs in a child process with tests/mocks/HiddenRenderer.h, and skips with
// the reason where there is no OpenGL driver, which is the CI runner today.
//
// Expected values come from the shop, not the menu: the sale moves the item to the
// shelves and pays the seller the shop's sell price out of the shopkeeper's gold.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SellMenuTest.*

#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>

#include "src/Colors.h"
#include "src/Controls.h"
#include "src/Creature.h"
#include "src/GameContext.h"
#include "src/InputSystem.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/MenuSell.h"
#include "src/Player.h"
#include "src/ShopKeeper.h"
#include "src/Vector2D.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// Ten is the average 3d6, which lets a directly built player carry a potion; at the
// Strength of 0 a direct build has, every item is too heavy.
constexpr int SELLER_STRENGTH = 10;

// One potion in the pack, a general store, and Enter pressed once: the potion is sold.
[[noreturn]] void sell_the_only_item_with_enter()
{
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	ctx.renderer = &hidden.renderer;

	Player seller{ Vector2D{ 1, 1 } };
	seller.set_strength(SELLER_STRENGTH);
	Creature shopkeeper{ Vector2D{ 2, 1 }, ActorData{ TileRef{}, "shopkeeper", ColorPairId::WHITE_BLACK } };
	shopkeeper.shop = std::make_unique<ShopKeeper>(ShopType::GENERAL_STORE, ShopQuality::AVERAGE, mock.dice);

	auto potion = ItemCreator::create("health_potion", Vector2D{ 0, 0 }, ctx);
	const int price = shopkeeper.shop->get_sell_price(*potion);
	exit_unless(InventoryOperations::add_item(seller.inventoryData, std::move(potion)).has_value(), "the potion did not fit the pack");
	shopkeeper.adjust_gold(price);
	const int sellerGoldBefore = seller.get_gold();
	const std::size_t shelvesBefore = shopkeeper.shop->get_shop_inventory().items.size();

	MenuSell menu{ shopkeeper, seller, ctx };
	ctx.inputSystem->queue_key(GameKey::ENTER, 0);
	menu.menu(ctx);

	exit_unless(seller.inventoryData.items.empty(), "the potion is still in the pack");
	exit_unless(shopkeeper.shop->get_shop_inventory().items.size() == shelvesBefore + 1, "the potion did not reach the shelves");
	exit_unless(seller.get_gold() == sellerGoldBefore + price, "the seller was not paid the shop's price");
	exit_unless(shopkeeper.get_gold() == 0, "the shopkeeper did not pay the price");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}

// Two items, Down pressed twice: the cursor wraps from the second row back to the
// first, and Enter sells the first item. Without the wrap the cursor runs past the last
// row, the next frame's draw clamps it to the second, and the second item is sold.
[[noreturn]] void the_cursor_wraps_from_the_last_row_to_the_first()
{
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx = mock.to_game_context();
	ctx.renderer = &hidden.renderer;

	Player seller{ Vector2D{ 1, 1 } };
	seller.set_strength(SELLER_STRENGTH);
	Creature shopkeeper{ Vector2D{ 2, 1 }, ActorData{ TileRef{}, "shopkeeper", ColorPairId::WHITE_BLACK } };
	shopkeeper.shop = std::make_unique<ShopKeeper>(ShopType::GENERAL_STORE, ShopQuality::AVERAGE, mock.dice);

	// The first row is the health potion and the second the mana potion, in the order added.
	for (const char* itemKey : { "health_potion", "mana_potion" })
	{
		auto item = ItemCreator::create(itemKey, Vector2D{ 0, 0 }, ctx);
		shopkeeper.adjust_gold(shopkeeper.shop->get_sell_price(*item));
		exit_unless(InventoryOperations::add_item(seller.inventoryData, std::move(item)).has_value(), "an item did not fit the pack");
	}

	MenuSell menu{ shopkeeper, seller, ctx };
	for (const GameKey key : { GameKey::DOWN, GameKey::DOWN, GameKey::ENTER })
	{
		ctx.inputSystem->queue_key(key, 0);
		menu.menu(ctx);
	}

	exit_unless(seller.inventoryData.items.size() == 1, "not exactly one item was sold");
	exit_unless(seller.inventoryData.items.front()->itemKey == "mana_potion", "the cursor did not wrap: the second item was sold");
	std::_Exit(HIDDEN_RENDERER_PASSED);
}
} // namespace

TEST(SellMenuTest, EnterSellsTheItemUnderTheCursor)
{
	const int childExitCode = exit_code_in_child(&sell_the_only_item_with_enter);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED) << "the child's check that failed is in its output above";
}

TEST(SellMenuTest, DownFromTheLastRowWrapsToTheFirst)
{
	const int childExitCode = exit_code_in_child(&the_cursor_wraps_from_the_last_row_to_the_first);

	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED) << "the child's check that failed is in its output above";
}

// end of file: SellMenuTest.cpp
