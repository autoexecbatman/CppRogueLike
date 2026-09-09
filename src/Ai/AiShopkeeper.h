#pragma once

#include "Ai.h"

class Creature;
struct GameContext;

class AiShopkeeper : public Ai
{
private:
	void update(Creature& owner, GameContext& ctx) override;
	void load(const json& j) override;
	void save(json& j) override;

public:
	// Opens the trade menu between a shopkeeper and the player. Called when the
	// player bumps a shopkeeper, which the attitude check has already allowed.
	static void open_trade(Creature& owner, Creature& player, GameContext& ctx);
};
