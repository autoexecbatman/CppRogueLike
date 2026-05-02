#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <raylib.h>
#include <span>
#include <string>
#include <utility>

#include "../Actor/Creature.h"
#include "../Actor/InventoryOperations.h"
#include "../Actor/Item.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Systems/MessageSystem.h"
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

			// Get correct sell price from shopkeeper's shop system
			int sellPrice = item->get_value();
			if (shopkeeper.shop != nullptr)
			{
				sellPrice = shopkeeper.shop->get_sell_price(*item);
			}

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
	menu_print(1, static_cast<int>(state) + 2, menu_get_string(state));
	if (currentState == state)
	{
		menu_highlight_off();
	}
}

void MenuSell::handle_sell(void* tradeWin, Creature& shopkeeper, Creature& seller, GameContext& ctx)
{
	if (InventoryOperations::is_inventory_empty(seller.inventoryData) ||
		currentState >= InventoryOperations::get_item_count(seller.inventoryData))
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Invalid selection.", true);
		return;
	}

	const Item* item = InventoryOperations::get_item_at(seller.inventoryData, currentState);
	if (!item)
	{
		ctx.messageSystem->log("Error: Attempted to sell a null item.");
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Error: Invalid item.", true);
		return;
	}

	// Use shopkeeper's pricing system if available
	int price = item->get_value();
	if (shopkeeper.shop != nullptr)
	{
		price = shopkeeper.shop->get_sell_price(*item);
	}

	if (shopkeeper.get_gold() >= price)
	{
		// Remove item from seller
		auto removed_item = InventoryOperations::remove_item_at(seller.inventoryData, currentState);
		if (removed_item.has_value())
		{
			shopkeeper.adjust_gold(-price);
			seller.adjust_gold(price);

			auto add_result = InventoryOperations::add_item(shopkeeper.inventoryData, std::move(*removed_item));
			if (!add_result.has_value())
			{
				assert(InventoryOperations::add_item(seller.inventoryData, std::move(*removed_item)).has_value());
				shopkeeper.adjust_gold(price);
				seller.adjust_gold(-price);
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "Shopkeeper's inventory is full.", true);
				return;
			}

			// Adjust currentState bounds
			if (currentState >= InventoryOperations::get_item_count(seller.inventoryData) && !InventoryOperations::is_inventory_empty(seller.inventoryData))
			{
				currentState = InventoryOperations::get_item_count(seller.inventoryData) - 1;
			}

			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Item sold successfully.", true);
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Transaction failed.", true);
		}
	}
	else
	{
		ctx.messageSystem->log("Shopkeeper does not have enough gold to buy the item.");
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Shopkeeper does not have enough gold to buy the item.", true);
	}
}

MenuSell::MenuSell(Creature& shopkeeper, Creature& player, GameContext& ctx)
	: player(player), shopkeeper(shopkeeper)
{
	if (ctx.renderer)
	{
		menuHeight = static_cast<size_t>(ctx.renderer->get_viewport_rows() - GUI_RESERVE_ROWS);
		menuWidth = static_cast<size_t>(ctx.renderer->get_viewport_cols());
	}
	else
	{
		menuHeight = 26;
		menuWidth = 60;
	}

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

	if (renderer)
	{
		int tileSize = renderer->get_tile_size();
		int font_off = (tileSize - renderer->get_font_size()) / 2;
		int hdr_x = (static_cast<int>(menuStartX) + 1) * tileSize;
		int hdr_y = (static_cast<int>(menuStartY) + 1) * tileSize + font_off;
		renderer->draw_text(Vector2D{ hdr_x, hdr_y }, "Item                       Price", CYAN_BLACK_PAIR);
	}

	draw_content();
	menu_refresh();
}

void MenuSell::on_key(int key, GameContext& ctx)
{
	switch (key)
	{

	case 0x103:
	case 'w':
	{
		// Don't allow navigation if no sellable items
		if (InventoryOperations::is_inventory_empty(player.inventoryData))
		{
			return;
		}

		if (menuItems.empty())
		{
			return; // Check for empty menu
		}

		currentState = (currentState + menuItems.size() - 1) % menuItems.size();
		break;
	}

	case 0x102:
	case 's':
	{
		// Don't allow navigation if no sellable items
		if (InventoryOperations::is_inventory_empty(player.inventoryData))
		{
			return;
		}
		if (menuItems.empty())
		{
			return; // Check for empty menu
		}
		currentState = (currentState + 1) % menuItems.size();
		break;
	}

	case 10: // Enter key
	{
		// Only allow selling if player actually has items
		if (!InventoryOperations::is_inventory_empty(player.inventoryData) && !menuItems.empty())
		{
			handle_sell(nullptr, shopkeeper, player, ctx);
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "No items to sell.", true);
		}
		break;
	}

	case 27: // Escape key
	{
		menu_set_run_false();
		break;
	}

	}
}

void MenuSell::menu(GameContext& ctx)
{
	menu_key_listen();
	draw();
	on_key(keyPress, ctx);
}
