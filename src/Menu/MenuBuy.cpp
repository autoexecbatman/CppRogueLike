#include "MenuBuy.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Colors/Colors.h"
#include "../Systems/MessageSystem.h"
#include "../ActorTypes/Player.h"
#include "../Actor/InventoryOperations.h"
#include "../Systems/ShopKeeper.h"

using namespace InventoryOperations;

void MenuBuy::populate_items()
{
	menuItems.clear();

	if (is_inventory_empty(shopkeeper.shop_inventory))
	{
		menuItems.push_back("No items for sale");
		return;
	}

	for (const auto& item : shopkeeper.shop_inventory.items)
	{
		if (item)
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
}

MenuBuy::MenuBuy(GameContext& ctx, Creature& buyer, ShopKeeper& shopkeeper)
	: buyer{ buyer }, shopkeeper{ shopkeeper }, ctx{ ctx }
{
	if (ctx.renderer)
	{
		menu_height = static_cast<size_t>(ctx.renderer->get_viewport_rows() - GUI_RESERVE_ROWS);
		menu_width  = static_cast<size_t>(ctx.renderer->get_viewport_cols());
	}
	else
	{
		menu_height = 26;
		menu_width  = 60;
	}

	populate_items();
	menu_new(menu_height, menu_width, menu_starty, menu_startx, ctx);
}

MenuBuy::~MenuBuy()
{
	menu_delete();
}

void MenuBuy::menu_print_state(size_t state)
{
	if (state >= menuItems.size()) return;

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
		int ts       = renderer->get_tile_size();
		int font_off = (ts - renderer->get_font_size()) / 2;
		int hdr_x    = (static_cast<int>(menu_startx) + 1) * ts;
		int hdr_y    = (static_cast<int>(menu_starty) + 1) * ts + font_off;
		renderer->draw_text(hdr_x, hdr_y, "Item                       Price", CYAN_BLACK_PAIR);
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
			if (menuItems.empty()) return;
			currentState = (currentState + menuItems.size() - 1) % menuItems.size();
			menu_mark_dirty();
			break;
		case 0x102: // DOWN
		case 's':
			if (menuItems.empty()) return;
			currentState = (currentState + 1) % menuItems.size();
			menu_mark_dirty();
			break;
		case 27: // ESC
			menu_set_run_false();
			break;
		case 10: // ENTER
			if (!is_inventory_empty(shopkeeper.shop_inventory))
			{
				handle_buy(nullptr, buyer, *ctx.player);
				menu_mark_dirty();
			}
			else
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "No items for sale.", true);
			}
			break;
	}
}

void MenuBuy::menu(GameContext& ctx)
{
	draw();

	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress, ctx);
	}
}

void MenuBuy::handle_buy(void* tradeWin, Creature& shopkeeper_creature, Player& buyer)
{
    if (is_inventory_empty(shopkeeper.shop_inventory) ||
        currentState >= get_item_count(shopkeeper.shop_inventory))
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "Invalid selection.", true);
        return;
    }

    Item* item = get_item_at(shopkeeper.shop_inventory, currentState);
    if (!item)
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "Invalid selection.", true);
        return;
    }

    if (shopkeeper.process_player_purchase(ctx, *item, buyer))
    {
        auto removed_item = remove_item_at(shopkeeper.shop_inventory, currentState);

        if (currentState >= get_item_count(shopkeeper.shop_inventory) && !is_inventory_empty(shopkeeper.shop_inventory))
        {
            currentState = get_item_count(shopkeeper.shop_inventory) - 1;
        }

        populate_items();
    }
}
