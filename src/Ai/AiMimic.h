#pragma once

#include "../Ai/AiMonster.h"
#include "../ActorTypes/Monsters.h"

class AiMimic : public AiMonster
{
public:
    AiMimic();
    void update(Creature& owner) override;
    void load(const json& j) override;
    void save(json& j) override;

    int revealDistance = 1;  // Reveal true form when player is this close
private:
    bool consumeNearbyItems(Mimic& mimic);
    void checkRevealing(Mimic& mimic);
    void changeDisguise(Mimic& mimic);

    // Fields moved from Mimic class
    bool isDisguised = true;
    int confusionDuration = 3; // Turns of confusion
    int itemsConsumed = 0;   // Track how many items this mimic has eaten

    int disguiseChangeCounter = 0;
    static constexpr int disguiseChangeRate = 200;
    int consumptionCooldown = 0;
};