#include <iostream>

#include "MenuSell.h"
#include "../Actor/Actor.h"
#include "../Actor/InventoryOperations.h"
#include "BaseMenu.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"

using namespace InventoryOperations; // For clean function calls without namespace prefix

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
			int sellPrice = item->value; // fallback
			if (shopkeeper.shop != nullptr)
			{
				sellPrice = shopkeeper.shop->get_sell_price(*item);
			}
			
			std::string goldText = "(" + std::to_string(sellPrice) + "g)";
			
			// Pad to align gold values (assuming max name length ~20)
			size_t totalWidth = 28;
			size_t padding = totalWidth > (itemName.length() + goldText.length()) ? 
							 totalWidth - itemName.length() - goldText.length() : 1;
			
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
	if (state >= menuItems.size()) return; // Bounds check
	
	if (currentState == state)
	{
		menu_highlight_on();
	}
	menu_print(2, static_cast<int>(state) + 4, menu_get_string(state)); // Start at row 4, indent from left border
	if (currentState == state)
	{
		menu_highlight_off();
	}
}

void MenuSell::handle_sell(void* tradeWin, Creature& shopkeeper, Creature& seller, GameContext& ctx)
{
    if (is_inventory_empty(seller.inventory_data) ||
        currentState >= get_item_count(seller.inventory_data))
    {
        ctx.message_system->message(WHITE_BLACK_PAIR, "Invalid selection.", true);
        return;
    }
    
    const Item* item = get_item_at(seller.inventory_data, currentState);
    if (!item)
    {
        ctx.message_system->log("Error: Attempted to sell a null item.");
        ctx.message_system->message(WHITE_BLACK_PAIR, "Error: Invalid item.", true);
        return;
    }
    
    // Use shopkeeper's pricing system if available
    int price = item->value; // fallback price
    if (shopkeeper.shop != nullptr)
    {
        price = shopkeeper.shop->get_sell_price(*item);
    }
    
    if (shopkeeper.get_gold() >= price)
    {
        // Remove item from seller
        auto removed_item = remove_item_at(seller.inventory_data, currentState);
        if (removed_item.has_value())
        {
            shopkeeper.adjust_gold(-price);
            seller.adjust_gold(price);
            
            auto add_result = add_item(shopkeeper.inventory_data, std::move(*removed_item));
            if (!add_result.has_value())
            {
                add_item(seller.inventory_data, std::move(*removed_item));
                shopkeeper.adjust_gold(price);
                seller.adjust_gold(-price);
                ctx.message_system->message(WHITE_BLACK_PAIR, "Shopkeeper's inventory is full.", true);
                return;
            }
            
            // Adjust currentState bounds
            if (currentState >= get_item_count(seller.inventory_data) && !is_inventory_empty(seller.inventory_data))
            {
                currentState = get_item_count(seller.inventory_data) - 1;
            }
            
            ctx.message_system->message(WHITE_BLACK_PAIR, "Item sold successfully.", true);
        }
        else
        {
            ctx.message_system->message(WHITE_BLACK_PAIR, "Transaction failed.", true);
        }
    }
    else
    {
        ctx.message_system->log("Shopkeeper does not have enough gold to buy the item.");
        ctx.message_system->message(WHITE_BLACK_PAIR, "Shopkeeper does not have enough gold to buy the item.", true);
    }
}

MenuSell::MenuSell(Creature& shopkeeper, Creature& player, GameContext& ctx) : player(player), shopkeeper(shopkeeper)
{
	// Use full screen dimensions
	menu_height = 30;
	menu_width = 119;
	
	// Player inventory is always initialized - no need to check
	populate_items(player.inventory_data.items);
	menu_new(menu_height, menu_width, menu_starty, menu_startx, ctx);
	
	// Reset currentState if inventory is empty
	if (is_inventory_empty(player.inventory_data))
	{
		currentState = 0;
	}
	else if (currentState >= player.inventory_data.items.size())
	{
		currentState = is_inventory_empty(player.inventory_data) ? 0 : player.inventory_data.items.size() - 1;
	}
}

MenuSell::~MenuSell()
{
	menu_delete();
}

void MenuSell::draw_content()
{
	// Player inventory is always initialized - no need to check
	populate_items(player.inventory_data.items);
	
	// Validate currentState after repopulating
	if (is_inventory_empty(player.inventory_data))
	{
		currentState = 0;
	}
	else if (currentState >= player.inventory_data.items.size())
	{
		currentState = player.inventory_data.items.size() - 1;
	}
	
	// Draw all menu items efficiently
	for (size_t i{ 0 }; i < menuItems.size(); ++i)
	{
		menu_print_state(i);
	}
}

void MenuSell::draw()
{
	// TODO: Reimplement with Panel+Renderer
	populate_items(player.inventory_data.items);
	if (player.inventory_data.items.empty())
	{
		currentState = 0;
	}
	else if (currentState >= player.inventory_data.items.size())
	{
		currentState = player.inventory_data.items.size() - 1;
	}
}

void MenuSell::on_key(int key, GameContext& ctx)
{
	switch (key)
	{
	case 0x103:
	case 'w':
		// Don't allow navigation if no sellable items
		if (is_inventory_empty(player.inventory_data)) return;
		if (menuItems.empty()) return; // Check for empty menu
		currentState = (currentState + menuItems.size() - 1) % menuItems.size();
		menu_mark_dirty(); // Mark for redraw
		break;
	case 0x102:
	case 's':
		// Don't allow navigation if no sellable items
		if (is_inventory_empty(player.inventory_data)) return;
		if (menuItems.empty()) return; // Check for empty menu
		currentState = (currentState + 1) % menuItems.size();
		menu_mark_dirty(); // Mark for redraw
		break;
	case 10: // Enter key
	{
		// Only allow selling if player actually has items
		if (!is_inventory_empty(player.inventory_data) && !menuItems.empty())
		{
			handle_sell(nullptr, shopkeeper, player, ctx);
			menu_mark_dirty(); // Redraw after sale
		}
		else
		{
			ctx.message_system->message(WHITE_BLACK_PAIR, "No items to sell.", true);
		}
	}
	break;
	case 27: // Escape key
	{
		menu_set_run_false();
	}
	break;
	}
}

void MenuSell::menu(GameContext& ctx)
{
	// Initial draw
	draw();
	
	while(run)
	{
		// Only redraw if needed (navigation/state change)
		draw();
		
		menu_key_listen();
		on_key(keyPress, ctx);
	}
	
	// TODO: screen clearing handled by Renderer
}
