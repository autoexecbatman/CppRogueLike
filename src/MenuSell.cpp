#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <span>
#include <string>
#include <utility>

#include "Creature.h"
#include "InventoryOperations.h"
#include "Item.h"
#include "Colors.h"
#include "GameContext.h"
#include "Renderer.h"
#include "MessageSystem.h"
#include "BaseMenu.h"
#include "MenuSell.h"

void MenuSell::populate_items(std::span<std::unique_ptr<Item>> item)
{
	menuItems.clear();

	// Handle empty inventory case
	if (item.empty())
	{
		menuItems.push_back("No items to sell");
		return;
	}

	for (const auto& item : item)
	{
		if (item)
		{
			// Display item name with right-aligned sell price
			std::string itemName = item->actorData.name;

			const int sellPrice = shopkeeper.shop->get_sell_price(*item);
			std::string goldText = "(" + std::to_string(sellPrice) + "g)";

			// Pad to align gold values (assuming max name length ~20)
			size_t totalWidth = 28;
			size_t padding = totalWidth > (itemName.length() + goldText.length()) ? totalWidth - itemName.length() - goldText.length() : 1;

			std::string itemDisplay = itemName + std::string(padding, ' ') + goldText;
			menuItems.push_back(itemDisplay);
		}
		else
		{
			std::cerr << "MenuSell Item is null." << std::endl;
			std::exit(EXIT_FAILURE);
		}
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
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Invalid selection.", true);
		return;
	}

	Item* item = InventoryOperations::get_item_at(seller.inventoryData, currentState);
	assert(item && "MenuSell: the pack holds a null item");

	// The shop decides and reports; this menu only keeps its cursor inside the pack.
	if (shopkeeper.shop->process_player_sale(ctx, *item, seller, shopkeeper)
		&& currentState >= InventoryOperations::get_item_count(seller.inventoryData)
		&& !InventoryOperations::is_inventory_empty(seller.inventoryData))
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
	for (size_t i{ 0 }; i < menuItems.size(); ++i)
	{
		menu_print_state(i);
	}
}

void MenuSell::draw()
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("SELL ITEMS", YELLOW_BLACK_PAIR);

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
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "No items to sell.", true);
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
