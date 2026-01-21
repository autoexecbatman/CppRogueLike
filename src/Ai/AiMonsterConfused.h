#pragma once

#include <memory>

#include "Ai.h"

class Creature;
struct GameContext;

class AiMonsterConfused : public Ai
{
public:
	AiMonsterConfused(int nbTurns, std::unique_ptr<Ai> oldAi) noexcept;
	void update(Creature& owner, GameContext& ctx) override;
	void load(const json& j) override;
	void save(json& j) override;

protected:
	int nbTurns;
	std::unique_ptr<Ai> oldAi;
};
