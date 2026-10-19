#pragma once

#include "Actor.h"
#include "AiMonster.h"
#include "GameContext.h"
#include "Persistent.h"

// Forward declarations
struct Vector2D;

class AiMonsterRanged : public AiMonster
{
protected:
	int maxRangeDistance{ 5 }; // Maximum range for ranged attacks
	int optimalDistance{ 3 }; // Preferred distance for ranged attackers

	void move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx) override;
	bool tryRangedAttack(Creature& owner, Vector2D targetPos, GameContext& ctx);

public:
	void update(Creature& owner, GameContext& ctx) override;

	// The name its save carries, so a load builds a ranged Ai and not a melee one.
	[[nodiscard]] AiType get_ai_type() const noexcept override { return AiType::RANGED_MONSTER; }

	void load(const json& j) override;
	void save(json& j) override;
};