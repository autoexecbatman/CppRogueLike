#pragma once

#include "Ai.h"

class Item;
class Player;
class Actor;
struct Vector2D;

class AiShopkeeper : public Ai
{
private:
	int moveCount{ 0 };
	bool hasApproachedPlayer{ false }; // Shopkeeper only approaches player once
	void update(Creature& owner, GameContext& ctx) override;
	// Shopkeepers are non-hostile - replaces dynamic_cast detection
	bool is_hostile() const override { return false; }
	void load(const json& j) override;
	void save(json& j) override;
	int calculate_step(int positionDifference);
	void moveToTarget(Actor& owner, Vector2D target, GameContext& ctx);
protected:
	void moveOrTrade(Creature& owner, Vector2D target, GameContext& ctx);
public:
	bool tradeMenuOpen{ false }; // Prevents multiple trade menu opens - now public
	void trade(Creature& shopkeeper, Creature& player, GameContext& ctx);
};
