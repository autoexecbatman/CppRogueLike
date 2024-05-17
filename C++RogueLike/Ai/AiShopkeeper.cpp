// file: AiShopkeeper.cpp
#include <curses.h>
#include "AiShopkeeper.h"
#include "../Game.h"
#include "../Random/RandomDice.h"
#include "../Actor/Actor.h"


constexpr auto TRACKING_TURNS = 3; // Used in AiShopkeeper::update()
constexpr auto MIN_TRADE_DISTANCE = 2;  // Minimum distance to initiate a trade

AiShopkeeper::AiShopkeeper() : moveCount(0) {}

// If positionDifference > 0, return 1; otherwise, return -1
int AiShopkeeper::calculateStep(int positionDifference)
{
	return positionDifference > 0 ? 1 : -1;
}

void AiShopkeeper::moveToTarget(Actor& owner, int targetX, int targetY)
{
	// Calculate how many squares away the target is horizontally and vertically
	int moveX = targetX - owner.position.x; // Number of squares to move horizontally
	int moveY = targetY - owner.position.y; // Number of squares to move vertically

	// Decide in which direction to take one step horizontally and vertically
	int stepX = calculateStep(moveX); // Direction to move horizontally (left or right)
	int stepY = calculateStep(moveY); // Direction to move vertically (up or down)

	// List potential moves in order of priority
	std::vector<std::pair<int, int>> moves
	{
		{stepX, stepY},  // First priority: move diagonally
		{stepX, 0},      // Second priority: move horizontally only
		{0, stepY}       // Third priority: move vertically only
	};

	// Try each move in order of priority until one is successful
	for (const auto& move : moves)
	{
		int nextX = owner.position.x + move.first;
		int nextY = owner.position.y + move.second;
		if (game.map->can_walk(nextX, nextY))
		{
			owner.position.x = nextX;
			owner.position.y = nextY;
			break; // Exit the loop after a successful move
		}
	}
}

void AiShopkeeper::moveOrTrade(Actor& owner, int targetx, int targety)
{
	const double distance = sqrt(pow(targetx - owner.position.x, 2) + pow(targety - owner.position.y, 2));

	if (distance >= MIN_TRADE_DISTANCE)
	{
		moveToTarget(owner, targetx, targety);
	}
	else
	{
		trade();
		// if traded kill shopkeeper dies
		owner.destructible->die(owner);
		owner.ai = nullptr;

		game.message(WHITE_PAIR, "The shopkeeper has been killed!", true);
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

void AiShopkeeper::display_item_list(WINDOW* tradeWin, std::span<std::unique_ptr<Actor>> inventoryList)
{
	for (size_t i = 0; i < inventoryList.size(); i++)
	{
		mvwprintw(tradeWin, 7 + i, 1, "%d: %s", i, inventoryList[i]->actorData.name.c_str());
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
		display_item_list(tradeWin, std::span(shopkeeperInventory->inventoryList));

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
void AiShopkeeper::handle_sell(WINDOW* tradeWin)
{
	const auto& playerInventory{ game.player->container };
	int selected{ 0 };
	bool selling{ true };

	while (selling)
	{
		// Display player's items for selling
		mvwprintw(tradeWin, 6, 1, "Items available for sale:");
		display_item_list(tradeWin, std::span(playerInventory->inventoryList));

		wrefresh(tradeWin);

		int choice = wgetch(tradeWin);

		if (choice >= '0' && choice <= '9')
		{
			selected = choice - '0';

			// Check if selected item is valid
			if (selected < playerInventory->inventoryList.size())
			{
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
	game.log("Shopkeeper AI update");
	if (owner.ai == nullptr || owner.destructible->is_dead()) // if the owner has no ai OR if the owner is dead
	{
		return; // do nothing
	}

	if (game.map->is_in_fov(owner.position)) // if the owner is in the fov
	{
		// move towards the player
		moveCount = TRACKING_TURNS;
	}
	else if (moveCount > 0) // if the move count is greater than 0
	{
		moveCount--; // decrement the move count
	}

	if (moveCount > 0) // if the move count is greater than 0
	{
		moveOrTrade(owner, game.player->position.x, game.player->position.y); // move or trade with the player
	}
}

void AiShopkeeper::load(TCODZip& zip)
{
}

void AiShopkeeper::save(TCODZip& zip)
{
}
// file: AiShopkeeper.cpp
