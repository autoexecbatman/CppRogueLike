#include <algorithm>
#include <cassert>
#include <string>

#include "../Actor/Actor.h"
#include "../Actor/InventoryOperations.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/ShopKeeper.h"
#include "MenuBuy.h"

void MenuBuy::populate_items()
{
	menuItems.clear();

	if (InventoryOperations::is_inventory_empty(shopkeeper.get_shop_inventory()))
	{
		menuItems.push_back("No items for sale");
		return;
	}

	assert(std::ranges::none_of(shopkeeper.get_shop_inventory().items, [](const auto& item) { return !item; }));
	for (const auto& item : shopkeeper.get_shop_inventory().items)
	{
		std::string itemName = item->actorData.name;
		int price = shopkeeper.get_buy_price(*item);
		std::string goldText = "(" + std::to_string(price) + "g)";

		size_t totalWidth = 28;
		size_t padding = totalWidth > (itemName.length() + goldText.length()) ? totalWidth - itemName.length() - goldText.length() : 1;

		std::string itemDisplay = itemName + std::string(padding, ' ') + goldText;
		menuItems.push_back(itemDisplay);
	}
}

MenuBuy::MenuBuy(GameContext& ctx, Creature& buyer, ShopKeeper& shopkeeper)
	: buyer{ buyer }, shopkeeper{ shopkeeper }, ctx{ ctx }
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

	populate_items();
	menu_new(
		menuWidth,
		menuHeight,
		menuStartX,
		menuStartY,
		ctx);
}

void MenuBuy::menu_print_state(size_t state)
{
	if (state >= menuItems.size())
		return;

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

void MenuBuy::draw_content()
{
	populate_items();
	for (size_t i{ 0 }; i < menuItems.size(); ++i)
	{
		menu_print_state(i);
	}
}

void MenuBuy::draw()
{
	menu_clear();
	menu_draw_box();
	menu_draw_title("BUY ITEMS", YELLOW_BLACK_PAIR);

	// Column header at row 1
	if (renderer)
	{
		int tileSize = renderer->get_tile_size();
		int font_off = (tileSize - renderer->get_font_size()) / 2;
		int hdr_x = (static_cast<int>(menuStartX) + 1) * tileSize;
		int hdr_y = (static_cast<int>(menuStartY) + 1) * tileSize + font_off;
		renderer->draw_text(Vector2D{ hdr_x, hdr_y }, "Item                       Price", CYAN_BLACK_PAIR);
	}

	populate_items();
	for (size_t i{ 0 }; i < menuItems.size(); ++i)
	{
		menu_print_state(i);
	}
	menu_refresh();
}

void MenuBuy::on_key(int key, GameContext& ctx)
{
	switch (key)
	{

	case 0x103: // UP
	case 'w':
	{
		if (menuItems.empty())
		{
			return;
		}
		currentState = (currentState + menuItems.size() - 1) % menuItems.size();
		break;
	}

	case 0x102: // DOWN
	case 's':
	{
		if (menuItems.empty())
		{
			return;
		}
		currentState = (currentState + 1) % menuItems.size();
		break;
	}

	case 27: // ESC
	{
		menu_set_run_false();
		break;
	}

	case 10: // ENTER
	{
		if (!InventoryOperations::is_inventory_empty(shopkeeper.get_shop_inventory()))
		{
			handle_buy();
		}
		else
		{
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "No items for sale.", true);
		}
		break;
	}

	}
}

void MenuBuy::menu(GameContext& ctx)
{
	menu_key_listen();
	draw();
	on_key(keyPress, ctx);
}

void MenuBuy::handle_buy()
{
	if (InventoryOperations::is_inventory_empty(shopkeeper.get_shop_inventory()) ||
		currentState >= InventoryOperations::get_item_count(shopkeeper.get_shop_inventory()))
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Invalid selection.", true);
		return;
	}

	Item* item = InventoryOperations::get_item_at(shopkeeper.get_shop_inventory(), currentState);
	if (!item)
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Invalid selection.", true);
		return;
	}

	if (shopkeeper.process_player_purchase(ctx, *item, buyer))
	{
		assert(InventoryOperations::remove_item_at(shopkeeper.get_shop_inventory(), currentState).has_value());

		if (currentState >= InventoryOperations::get_item_count(shopkeeper.get_shop_inventory()) && !InventoryOperations::is_inventory_empty(shopkeeper.get_shop_inventory()))
		{
			currentState = InventoryOperations::get_item_count(shopkeeper.get_shop_inventory()) - 1;
		}

		populate_items();
	}
}
