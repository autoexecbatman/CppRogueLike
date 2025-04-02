#pragma once

#include "Ai/AiMonster.h"
#include "ActorTypes/Monsters.h"

class AiMimic : public AiMonster
{
public:
    AiMimic();
    void update(Creature& owner) override;
    void load(const json& j) override;
    void save(json& j) override;

private:
    bool consumeNearbyItems(Mimic& mimic);
    void checkRevealing(Mimic& mimic);
    void changeDisguise(Mimic& mimic);

    int disguiseChangeCounter = 0;
    static constexpr int disguiseChangeRate = 200;
    int consumptionCooldown = 0;
};