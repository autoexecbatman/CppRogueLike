#pragma once

#include "AiMonster.h"

// Forward declarations
struct Vector2D;

class AiMonsterRanged : public AiMonster
{
public:
    AiMonsterRanged();
    void update(Creature& owner) override;
    void load(const json& j) override;
    void save(json& j) override;

protected:
    int maxRangeDistance = 5; // Maximum range for ranged attacks
    int optimalDistance = 3;  // Preferred distance for ranged attackers

    void moveOrAttack(Creature& owner, Vector2D targetPosition);
    bool tryRangedAttack(Creature& owner, Vector2D targetPos);
    void animateProjectile(Vector2D from, Vector2D to, char projectileChar);
};