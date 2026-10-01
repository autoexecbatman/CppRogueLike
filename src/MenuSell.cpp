// file: MenuSell.cpp
//
// The screen a player sells from, inside a shop.
//
// One line per item in the player's pack, each showing what the shopkeeper would pay
// for it. Enter sells whatever the cursor is on, Escape leaves. The menu owns no part
// of the transaction: handle_sell keeps the cursor inside the pack and hands the item
// to the shop, which decides the price, moves the gold and says what happened.
//
// Like every menu here it is frame-based - menu() does one frame of work and returns,
// because a blocking loop hangs the browser under Emscripten. Push it onto ctx.menus
// and MenuManager ticks it.
//
// Usage:
//
//   ctx.menus->push_back(std::make_unique<MenuSell>(shopkeeper, player, ctx));
//
// The shopkeeper has to carry a shop component and the renderer has to exist before
// construction; both are asserted, because every way into this menu goes through a
// shop that already opened.

#include <cassert>
#include <memory>
#include <raylib.h>
#include <span>
#include <string>
#include <utility>

#include "BaseMenu.h"
#include "Colors.h"
#include "Creature.h"
#include "GameContext.h"
#include "InventoryOperations.h"
#include "Item.h"
#include "MenuSell.h"
#include "MessageSystem.h"
#include "Renderer.h"

// Builds the one line per item that this menu displays, each carrying the name and
// what the shop would pay for it.
//
// An empty pack is a legal thing to open the sell menu with: it produces the single
// line "No items to sell" rather than no lines, so the menu has something to draw and
// the cursor has somewhere to sit. The prices come from the shopkeeper, which is why
// the constructor refuses a creature with no shop. It clears first, so the caller may
// run it every frame.
//
// Example:
//   populate_items(player.inventoryData.items);  // the player is carrying two items
//                                                // menuItems now holds two lines, each the
//                                                // item's name padded out to column 28 and
//                                                // then the shop's price in gold
//
//   populate_items({});                          // menuItems holds one line,
//                                                // "No items to sell"
void MenuSell::populate_items(std::span<std::unique_ptr<Item>> items)
{
	menuItems.clear();

	// An empty pack still needs a line, or the menu draws nothing and the cursor has
	// no row to sit on.
	if (items.empty())
	{
		menuItems.push_back("No items to sell");
		return;
	}

	for (const auto& item : items)
	{
		assert(item && "populate_items: the pack holds a null where an item should be");

		std::string itemName = item->actorData.name;

		const int sellPrice = shopkeeper.shop->get_sell_price(*item);
		std::string goldText = "(" + std::to_string(sellPrice) + "g)";

		// The gold figures line up by padding the name out to a fixed column. A name
		// long enough to reach it keeps one space, so the two never run together.
		const size_t totalWidth = 28;
		const size_t padding = totalWidth > (itemName.length() + goldText.length()) ? totalWidth - itemName.length() - goldText.length() : 1;

		menuItems.push_back(itemName + std::string(padding, ' ') + goldText);
	}
}

void MenuSell::menu_print_state(size_t state)
{
	if (state >= menuItems.size())
	{
		return;
	}
	if (currentState == state)
	{
		menu_highlight_on();
	}
	menu_print(1, static_cast<int>(state) + 1, menu_get_string(state));
	if (currentState == state)
	{
		menu_highlight_off();
	}
}

void MenuSell::handle_sell(Creature& shopkeeper, Creature& seller, GameContext& ctx)
{
	if (InventoryOperations::is_inventory_empty(seller.inventoryData) ||
		currentState >= InventoryOperations::get_item_count(seller.inventoryData))
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Invalid selection.", MessageCompletion::FINISHED);
		return;
	}

	Item* item = InventoryOperations::get_item_at(seller.inventoryData, currentState);
	assert(item && "MenuSell: the pack holds a null item");

	// The shop decides and reports; this menu only keeps its cursor inside the pack.
	if (shopkeeper.shop->process_player_sale(ctx, *item, seller, shopkeeper) && currentState >= InventoryOperations::get_item_count(seller.inventoryData) && !InventoryOperations::is_inventory_empty(seller.inventoryData))
	{
		currentState = InventoryOperations::get_item_count(seller.inventoryData) - 1;
	}
}

MenuSell::MenuSell(Creature& shopkeeper, Creature& player, GameContext& ctx)
	: player(player), shopkeeper(shopkeeper)
{
	assert(ctx.renderer && "MenuSell: renderer required before construction");
	// Every way into the trade menu requires a shop, so there is always one to sell to.
	assert(shopkeeper.shop && "MenuSell opened on a creature with no shop");
	menuHeight = static_cast<size_t>(ctx.renderer->get_viewport_rows() - ctx.renderer->get_gui_reserve_rows());
	menuWidth = static_cast<size_t>(ctx.renderer->get_viewport_cols());

	populate_items(player.inventoryData.items);
	menu_new(menuWidth, menuHeight, menuStartX, menuStartY, ctx);

	if (InventoryOperations::is_inventory_empty(player.inventoryData))
	{
		currentState = 0;
	}
	else if (currentState >= player.inventoryData.items.size())
	{
		currentState = player.inventoryData.items.size() - 1;
	}
}

void MenuSell::draw_content()
{
	// Player inventory is always initialized - no need to check
	populate_items(player.inventoryData.items);

	// Validate currentState after repopulating
	if (InventoryOperations::is_inventory_empty(player.inventoryData))
	{
		currentState = 0;
	}
	else if (currentState >= player.inventoryData.items.size())
	{
		currentState = player.inventoryData.items.size() - 1;
	}

	// Draw all menu items efficiently
	for (size_t row{ 0 }; row < menuItems.size(); ++row)
	{
		menu_print_state(row);
	}
}

void MenuSell::draw()
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("SELL ITEMS", ColorPairId::YELLOW_BLACK);

	menu_print_header();

	draw_content();
	menu_refresh();
}

void MenuSell::on_key(GameContext& ctx)
{
	if (lastKey == GameKey::UP || lastKey == GameKey::W)
	{
		if (InventoryOperations::is_inventory_empty(player.inventoryData))
		{
			return;
		}
		if (menuItems.empty())
		{
			return;
		}
		currentState = (currentState + menuItems.size() - 1) % menuItems.size();
	}
	else if (lastKey == GameKey::DOWN || lastKey == GameKey::S)
	{
		if (InventoryOperations::is_inventory_empty(player.inventoryData))
		{
			return;
		}
		if (menuItems.empty())
		{
			return;
		}
		currentState = (currentState + 1) % menuItems.size();
	}
	else if (lastKey == GameKey::ENTER)
	{
		if (!InventoryOperations::is_inventory_empty(player.inventoryData) && !menuItems.empty())
		{
			handle_sell(shopkeeper, player, ctx);
		}
		else
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "No items to sell.", MessageCompletion::FINISHED);
		}
	}
	else if (lastKey == GameKey::ESCAPE)
	{
		menu_set_run_false();
	}
}

void MenuSell::menu(GameContext& ctx)
{
	menu_key_listen();
	draw();
	on_key(ctx);
}
