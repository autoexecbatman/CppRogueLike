#pragma once

#include "Ai.h"
#include "../Persistent/Persistent.h"

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

    virtual void move_or_attack(Creature& owner, Vector2D position, GameContext& ctx);
};
