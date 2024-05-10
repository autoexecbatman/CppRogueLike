// file: AiShopkeeper.h
#pragma once

#include "Ai.h"

class AiShopkeeper : public Ai
{
private:
	int moveCount = 0;
	void update(Actor& owner) override;
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;
	void handle_buy(WINDOW* tradeWin);
	void handle_sell(WINDOW* tradeWin);
	void display_item_list(WINDOW* tradeWin, std::vector<std::unique_ptr<Actor>>& inventoryList);
	int calculateStep(int positionDifference);
	void moveToTarget(Actor& owner, int targetx, int targety);
public:
	AiShopkeeper();
protected:
	void moveOrTrade(Actor& owner, int targetx, int targety);
	void trade();
};
// file: AiShopkeeper.h
