// file: AiShopkeeper.cpp
#include <curses.h>
#include "AiShopkeeper.h"
#include "../Game.h"
#include "../Random/RandomDice.h"
#include "../Actor/Actor.h"
#include "../Items.h"
#include "../MenuTrade.h"


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
		if (game.map->can_walk(Vector2D{ nextY, nextX }))
		{
			owner.position.x = nextX;
			owner.position.y = nextY;
			break; // Exit the loop after a successful move
		}
	}
}

void AiShopkeeper::moveOrTrade(Creature& shopkeeper, int targetx, int targety)
{
	const double distance = sqrt(pow(targetx - shopkeeper.position.x, 2) + pow(targety - shopkeeper.position.y, 2));

	if (distance >= MIN_TRADE_DISTANCE)
	{
		moveToTarget(shopkeeper, targetx, targety);
	}
	else
	{
		// get the target
		auto& player = *game.get_actor(Vector2D{ targety,targetx });
		if (!shopkeeper.destructible->is_dead()) { trade(shopkeeper, player); }
	}
}

void AiShopkeeper::trade(Creature& shopkeeper, Creature& player)
{
	game.menus.push_back(std::make_unique<MenuTrade>(shopkeeper, player));
}

void AiShopkeeper::display_item_list(WINDOW* tradeWin, std::span<std::unique_ptr<Item>> inventoryList)
{
	for (size_t i = 0; i < inventoryList.size(); i++)
	{
		mvwprintw(tradeWin, 7 + i, 1, "%d: %s", i, inventoryList[i]->actorData.name.c_str());
	}
}


void AiShopkeeper::handle_buy(WINDOW* tradeWin, Creature& shopkeeper, Creature& player)
{
	const auto& shopkeeperInventory{ shopkeeper.container };
	int selected{ 0 };
	bool buying{ true };

	while (buying)
	{
		// Display available items for buying
		mvwprintw(tradeWin, 6, 1, "Items available for purchase:");
		display_item_list(tradeWin, std::span(shopkeeperInventory->inv));

		wrefresh(tradeWin);

		int choice = wgetch(tradeWin);

		if (choice >= '0' && choice <= '9') {
			selected = choice - '0';

			// Check if selected item is valid
			if (selected < shopkeeperInventory->inv.size())
			{
				auto& item = shopkeeperInventory->inv[selected];

				// Check if player has enough currency
				if (game.player->playerGold >= item->value)
				{
					// Deduct currency
					game.player->playerGold -= item->value;

					// Transfer item from shopkeeper to player
					game.player->container->inv.insert(game.player->container->inv.begin(), std::move(item));

					shopkeeperInventory->inv.erase(shopkeeperInventory->inv.begin() + selected);
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
		display_item_list(tradeWin, std::span(playerInventory->inv));

		wrefresh(tradeWin);

		int choice = wgetch(tradeWin);

		if (choice >= '0' && choice <= '9')
		{
			selected = choice - '0';

			// Check if selected item is valid
			if (selected < playerInventory->inv.size())
			{
				auto& item = playerInventory->inv[selected];

				// Add currency to the player's total
				game.player->playerGold += item->value;

				// Transfer item from player to shopkeeper
				/*game.shopkeeper->container->add(std::move(item));*/

				playerInventory->inv.erase(playerInventory->inv.begin() + selected);
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

void AiShopkeeper::update(Creature& owner)
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
