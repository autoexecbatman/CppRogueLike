// file: AiShopkeeper.cpp
#include <curses.h>
#include "AiShopkeeper.h"
#include "../Game.h"
#include "../Random/RandomDice.h"
#include "../Actor/Actor.h"


constexpr auto TRACKING_TURNS = 3; // Used in AiShopkeeper::update()

AiShopkeeper::AiShopkeeper() : moveCount(0) {}

void AiShopkeeper::moveOrTrade(Actor& owner, int targetx, int targety)
{
	int dx = targetx - owner.posX; // get the x distance
	int dy = targety - owner.posY; // get the y distance

	const int stepdx = (dx > 0 ? 1 : -1); // get the x step
	const int stepdy = (dy > 0 ? 1 : -1); // get the y step

	const double distance = sqrt(dx * dx + dy * dy); // get the distance

	if (distance >= 2)
	{
		dx = static_cast<int>(round(dx / distance));
		dy = static_cast<int>(round(dy / distance));

		if (game.map != nullptr)
		{
			if (game.map->can_walk(owner.posX + dx, owner.posY + dy))
			{
				owner.posX += dx;
				owner.posY += dy;
			}
			else if (game.map->can_walk(owner.posX + stepdx, owner.posY))
			{
				owner.posX += stepdx;
			}
			else if (game.map->can_walk(owner.posX, owner.posY + stepdy))
			{
				owner.posY += stepdy;
			}
		}
		else
		{
			game.log("Error: game.map is null");
			exit(-1);
		}

	}
	else
	{
		trade();
	}
}

void AiShopkeeper::trade() {
	WINDOW* tradeWin = newwin(10, 40, 5, 5);
	box(tradeWin, 0, 0);
	wrefresh(tradeWin);

	bool trading = true;

	while (trading) {
		// Display the trade menu
		mvwprintw(tradeWin, 1, 1, "Trade Menu");
		mvwprintw(tradeWin, 2, 1, "1. Buy");
		mvwprintw(tradeWin, 3, 1, "2. Sell");
		mvwprintw(tradeWin, 4, 1, "3. Exit");

		wrefresh(tradeWin);

		// Get user input
		int choice = wgetch(tradeWin);

		switch (choice) {
		case '1':
			handle_buy(tradeWin);
			break;

		case '2':
			handle_sell(tradeWin);
			break;

		case '3':
			trading = false;
			break;

		default:
			break;
		}

		wrefresh(tradeWin);
	}

	delwin(tradeWin);
}

void AiShopkeeper::display_item_list(WINDOW* tradeWin, std::vector<std::unique_ptr<Actor>>& inventoryList)
{
	for (size_t i = 0; i < inventoryList.size(); i++)
	{
		mvwprintw(tradeWin, 7 + i, 1, "%d: %s", i, inventoryList[i]->name.c_str());
	}
}

void AiShopkeeper::handle_buy(WINDOW* tradeWin)
{
	const auto& shopkeeperInventory{ game.shopkeeper->container };
	int selected{ 0 };
	bool buying{ true };

	while (buying)
	{
		// Display available items for buying
		mvwprintw(tradeWin, 6, 1, "Items available for purchase:");
		display_item_list(tradeWin, shopkeeperInventory->inventoryList);

		wrefresh(tradeWin);

		int choice = wgetch(tradeWin);

		if (choice >= '0' && choice <= '9') {
			selected = choice - '0';

			// Check if selected item is valid
			if (selected < shopkeeperInventory->inventoryList.size())
			{
				auto& item = shopkeeperInventory->inventoryList[selected];

				// Check if player has enough currency
				if (game.player->playerGold >= item->value)
				{
					// Deduct currency
					game.player->playerGold -= item->value;

					// Transfer item from shopkeeper to player
					game.player->container->inventoryList.insert(game.player->container->inventoryList.begin(), std::move(item));

					shopkeeperInventory->inventoryList.erase(shopkeeperInventory->inventoryList.begin() + selected);
					mvwprintw(tradeWin, 9, 1, "Item purchased successfully.");
					buying = false;
				}
				else
				{
					mvwprintw(tradeWin, 9, 1, "Insufficient currency.");
					buying = false;
				}
			}
		}

		wrefresh(tradeWin);
	}
}

// Selling logic: display player's items and allow selling
void AiShopkeeper::handle_sell(WINDOW* tradeWin) {
	const auto& playerInventory = game.player->container;
	int selected = 0;
	bool selling = true;

	while (selling) {
		// Display player's items for selling
		mvwprintw(tradeWin, 6, 1, "Items available for sale:");
		display_item_list(tradeWin, playerInventory->inventoryList);

		wrefresh(tradeWin);

		int choice = wgetch(tradeWin);

		if (choice >= '0' && choice <= '9') {
			selected = choice - '0';

			// Check if selected item is valid
			if (selected < playerInventory->inventoryList.size()) {
				auto& item = playerInventory->inventoryList[selected];

				// Add currency to the player's total
				game.player->playerGold += item->value;

				// Transfer item from player to shopkeeper
				game.shopkeeper->container->add(std::move(item));

				playerInventory->inventoryList.erase(playerInventory->inventoryList.begin() + selected);
				mvwprintw(tradeWin, 9, 1, "Item sold successfully.");
				selling = false;
			}
		}
		else
		{
			mvwprintw(tradeWin, 9, 1, "Invalid selection.");
			selling = false;
		}

		wrefresh(tradeWin);
	}
}

void AiShopkeeper::update(Actor& owner)
{
	if (owner.ai == nullptr) // if the owner has no ai
	{
		return; // do nothing
	}

	if (owner.destructible != nullptr)
	{
		if (owner.destructible->is_dead()) // if the owner is dead
		{
			return; // do nothing
		}
	}
	else
	{
		std::cout << "Error: AiMonster::update() - owner.destructible is null" << std::endl;
		exit(-1);
	}

	if (game.map->is_in_fov(owner.posX, owner.posY)) // if the owner is in the fov
	{
		// move towards the player
		moveCount = TRACKING_TURNS;
	}
	else
	{
		moveCount--; // decrement the move count
	}

	if (moveCount > 0) // if the move count is greater than 0
	{
		moveOrTrade(owner, game.player->posX, game.player->posY); // move or trade with the player
	}
}

void AiShopkeeper::load(TCODZip& zip)
{
}

void AiShopkeeper::save(TCODZip& zip)
{
}
// file: AiShopkeeper.cpp
