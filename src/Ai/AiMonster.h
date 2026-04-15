#pragma once

#include "../Persistent/Persistent.h"
#include "Ai.h"

class Creature;
struct GameContext;
struct Vector2D;

inline constexpr int TRACKING_TURNS = 3; // Used in AiSpider::update()

class AiMonster : public Ai
{
private:
	static void flee(Creature& owner, GameContext& ctx);
	static void check_morale(Creature& owner, GameContext& ctx);

protected:
	int moveCount{ 0 };

	static bool blocked_by_sanctuary(GameContext& ctx);
	virtual void move_or_attack(Creature& owner, Vector2D position, GameContext& ctx);

public:
	void update(Creature& owner, GameContext& ctx) override;
	void load(const json& j) override;
	void save(json& j) override;
};
