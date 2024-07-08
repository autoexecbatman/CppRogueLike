// file: AiShopkeeper.h
#pragma once

#include <span>

#include "Ai.h"

class Item;

class AiShopkeeper : public Ai
{
private:
	int moveCount = 0;
	void update(Creature& owner) override;
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;
	void handle_buy(WINDOW* tradeWin, Creature& shopkeeper, Creature& player);
	void handle_sell(WINDOW* tradeWin);
	void display_item_list(WINDOW* tradeWin, std::span<std::unique_ptr<Item>> inventoryList);
	int calculateStep(int positionDifference);
	void moveToTarget(Actor& owner, int targetx, int targety);
public:
	AiShopkeeper();
protected:
	void moveOrTrade(Creature& owner, int targetx, int targety);
	void trade(Creature& buyer, Creature& player);
};
// file: AiShopkeeper.h
