// file: AiShopkeeper.h
#pragma once

#include <span>

#include "Ai.h"
#include "../Vector2D.h"

class Item;
class Player;
class Actor;

class AiShopkeeper : public Ai
{
private:
	int moveCount = 0;
	int cooldownCount = 0; // Cooldown turns after interaction  
	bool tradeMenuOpen = false; // Prevents multiple trade menu opens
	void update(Creature& owner) override;
	void load(const json& j) override;
	void save(json& j) override;
	int calculate_step(int positionDifference);
	void moveToTarget(Actor& owner, Vector2D target);
public:
	AiShopkeeper();
protected:
	void moveOrTrade(Creature& owner, Vector2D target);
public:
	void trade(Creature& shopkeeper, Creature& player);
};
// file: AiShopkeeper.h
