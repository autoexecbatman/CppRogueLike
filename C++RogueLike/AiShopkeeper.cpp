#include <curses.h>
#include "AiShopkeeper.h"
#include "RandomDice.h"
#include "Actor.h"
#include "Game.h"


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

void AiShopkeeper::trade()
{
	// create a window for the trade
	WINDOW* tradeWindow = newwin(10, 20, 1, 1);
	box(tradeWindow, 0, 0);
	wrefresh(tradeWindow);

	// find shopkeeper's inventory
	/*auto shopkeeperInventory = game.shopkeeper->container->inventoryList;*/
	// display shopkeeper's inventory
	//for (auto& item : shopkeeperInventory)
	//{
	//	wprintw(tradeWindow, item->name.c_str());
	//	wprintw(tradeWindow, "\n");
	//}
	refresh();
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
		moveOrTrade(owner, game.player->posX, game.player->posY); // move or attack the player
	}
}

void AiShopkeeper::load(TCODZip& zip)
{
}

void AiShopkeeper::save(TCODZip& zip)
{
}
// end of file: AiShopkeeper.cpp
