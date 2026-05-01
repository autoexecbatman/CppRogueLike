#pragma once

#include "Ai.h"

class Creature;
struct GameContext;

class AiShopkeeper : public Ai
{
private:
	void update(Creature& owner, GameContext& ctx) override;
	// Shopkeepers are non-hostile - replaces dynamic_cast detection
	bool is_hostile() const override { return false; }
	void load(const json& j) override;
	void save(json& j) override;

public:
	void open_trade(Creature& owner, Creature& player, GameContext& ctx) override;
};
