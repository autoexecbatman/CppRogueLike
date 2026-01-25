#pragma once
#include "../Ai/AiMonster.h"

class Mimic;
struct GameContext;
enum class ItemClass;

class AiMimic : public AiMonster
{
public:
    void update(Creature& owner, GameContext& ctx) override;
    void load(const json& j) override;
    void save(json& j) override;

private:
    [[nodiscard]] bool consume_nearby_items(Mimic& mimic, GameContext& ctx);
    void check_revealing(Mimic& mimic, GameContext& ctx);
    void change_disguise(Mimic& mimic, GameContext& ctx);
    
    // Helper methods for item consumption bonuses
    void apply_item_bonus(Mimic& mimic, ItemClass itemClass, GameContext& ctx);
    void boost_health(Mimic& mimic, int amount, GameContext& ctx);
    void boost_defense(Mimic& mimic, int amount, int maxDR, GameContext& ctx);
    void boost_attack(Mimic& mimic, GameContext& ctx);
    void boost_confusion_power(GameContext& ctx);
    void transform_to_greater_mimic(Mimic& mimic, GameContext& ctx);

    // Configuration constants (NOT serialized - same for all mimics)
    static constexpr int DISGUISE_CHANGE_RATE = 200;
    static constexpr int CONSUMPTION_COOLDOWN_TURNS = 3;
    static constexpr int CONSUMPTION_RADIUS = 1;
    static constexpr int MAX_CONFUSION_DURATION = 5;
    static constexpr int MAX_GOLD_DR_BONUS = 2;
    static constexpr int MAX_WEAPON_DAMAGE = 6;
    static constexpr int MAX_ARMOR_DR_BONUS = 3;
    static constexpr int ITEMS_FOR_TRANSFORMATION = 5;
    static constexpr int HEALTH_BONUS = 1;
    static constexpr int DR_BONUS = 1;
    static constexpr int CONFUSION_BONUS = 1;

    // Runtime state (IS serialized - unique per mimic instance)
    bool isDisguised{ true };
    int confusionDuration{ 3 };
    int itemsConsumed{ 0 };
    int disguiseChangeCounter{ 0 };
    int consumptionCooldown{ 0 };
    int revealDistance{ 1 };  // Public if needs external access
};