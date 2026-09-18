#pragma once

#include "Persistent.h"
#include "Ai.h"

class Creature;
struct GameContext;
struct Vector2D;


class AiMonster : public Ai
{
private:
	void update_tracking(Creature& owner, const GameContext& ctx);
	void decide_action(Creature& owner, GameContext& ctx);

protected:

	virtual void move_or_attack(Creature& owner, Vector2D position, GameContext& ctx);

public:
	void update(Creature& owner, GameContext& ctx) override;
	void load(const json& j) override;
	void save(json& j) override;
};
