#pragma once

#include "../Persistent/Persistent.h"
#include "Ai.h"

class Creature;
struct GameContext;
struct Vector2D;

inline constexpr int TRACKING_TURNS = 3; // Used in AiMonster::update()

class AiMonster : public Ai
{
public:
	void update(Creature& owner, GameContext& ctx) override;
	void load(const json& j) override;
	void save(json& j) override;

protected:
	int moveCount{ 0 };

	// AD&D 2e: Returns true if the player's Sanctuary spell blocks this monster's turn.
	static bool blocked_by_sanctuary(GameContext& ctx);

	virtual void move_or_attack(Creature& owner, Vector2D position, GameContext& ctx);

private:
	// AD&D 2e: Move away from player using inverted Dijkstra gradient.
	static void flee(Creature& owner, GameContext& ctx);

	// AD&D 2e: Roll 2d10 against morale score; set IS_FLEEING on failure.
	static void check_morale(Creature& owner, GameContext& ctx);
};
