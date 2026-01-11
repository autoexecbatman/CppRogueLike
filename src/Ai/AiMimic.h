#pragma once

#include "../Ai/AiMonster.h"
#include "../ActorTypes/Monsters.h"

struct GameContext; // Forward declaration

class AiMimic : public AiMonster
{
public:
    AiMimic();
    void update(Creature& owner, GameContext& ctx) override;
    void load(const json& j) override;
    void save(json& j) override;

    int revealDistance = 1;  // Reveal true form when player is this close
private:
    bool consume_nearby_items(Mimic& mimic, GameContext& ctx);
    void check_revealing(Mimic& mimic, GameContext& ctx);
    void change_disguise(Mimic& mimic, GameContext& ctx);

    // Fields moved from Mimic class
    bool isDisguised = true;
    int confusionDuration = 3; // Turns of confusion
    int itemsConsumed = 0;   // Track how many items this mimic has eaten

    int disguiseChangeCounter = 0;
    static constexpr int disguiseChangeRate = 200;
    int consumptionCooldown = 0;
};