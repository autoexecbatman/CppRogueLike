#pragma once

#include "AiMonster.h"

// Forward declarations
struct Vector2D;

class AiMonsterRanged : public AiMonster
{
public:
    void update(Creature& owner, GameContext& ctx) override;
    void load(const json& j) override;
    void save(json& j) override;
protected:
    int maxRangeDistance{ 5 }; // Maximum range for ranged attacks
    int optimalDistance{ 3 };  // Preferred distance for ranged attackers

    void move_or_attack(Creature& owner, Vector2D targetPosition, GameContext& ctx) override;
    bool tryRangedAttack(Creature& owner, Vector2D targetPos, GameContext& ctx);
    void animateProjectile(Vector2D from, Vector2D to, char projectileChar, GameContext& ctx);
};