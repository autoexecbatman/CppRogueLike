#pragma once

#include "Ai.h"
#include "../Persistent/Persistent.h"

class Creature;
struct GameContext;
struct Vector2D;

inline constexpr auto TRACKING_TURNS = 3; // Used in AiMonster::update()

class AiMonster : public Ai
{
public:
    AiMonster();
    void update(Creature& owner, GameContext& ctx) override;

    void load(const json& j) override;
    void save(json& j) override;

protected:
    int moveCount = 0;

    // Make this method virtual so it can be properly overridden
    virtual void moveOrAttack(Creature& owner, Vector2D position, GameContext& ctx);
};
