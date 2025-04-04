// file: AiMonster.h
#ifndef AI_MONSTER_H
#define AI_MONSTER_H

#pragma once

#include "Ai.h"
#include "../Persistent/Persistent.h"
#include "../Actor/Actor.h"

inline constexpr auto TRACKING_TURNS = 3; // Used in AiMonster::update()

class AiMonster : public Ai
{
public:
    AiMonster();
    void update(Creature& owner) override;

    void load(const json& j) override;
    void save(json& j) override;

protected:
    int moveCount = 0;

    // Make this method virtual so it can be properly overridden
    virtual void moveOrAttack(Creature& owner, Vector2D position);
};

#endif // !AI_MONSTER_H
// file: AiMonster.h
