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
};
