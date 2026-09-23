#pragma once

#include "Ai.h"

class Creature;
struct GameContext;

class AiShopkeeper : public Ai
{
private:
	void update(Creature& owner, GameContext& ctx) override;

	[[nodiscard]] AiType get_ai_type() const noexcept override { return AiType::SHOPKEEPER; }
	void load(const json& j) override;
	void save(json& j) override;
};
