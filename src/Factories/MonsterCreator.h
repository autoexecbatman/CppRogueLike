#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "../Combat/DamageInfo.h"
#include "../Systems/TargetMode.h"  // for ScrollAnimation, not needed but TargetMode pulls nothing bad

// Forward declarations
class Creature;
struct Vector2D;
struct GameContext;

enum class MonsterId
{
    GOBLIN,
    ORC,
    TROLL,
    DRAGON,
    ARCHER,
    MAGE,
    WOLF,
    FIRE_WOLF,
    ICE_WOLF,
    BAT,
    KOBOLD,
};

enum class MonsterAiType
{
    MELEE,
    RANGED,
};

// Dice expression: roll num dice of sides sides, add bonus. num=0 means skip.
struct DiceExpr
{
    int num{ 0 };
    int sides{ 0 };
    int bonus{ 0 };
};

struct MonsterParams
{
    // Identity
    char symbol{};
    std::string name;
    int color{ 0 };
    std::string corpse_name;

    // Combat
    DiceExpr hp_dice{};
    int thaco{ 20 };
    int ac{ 10 };
    int xp{ 0 };
    int dr{ 0 };

    // Ability scores — all six; default 3d6 (set num=0 to skip)
    DiceExpr str_dice{ 3, 6, 0 };
    DiceExpr dex_dice{ 3, 6, 0 };
    DiceExpr con_dice{ 3, 6, 0 };
    DiceExpr int_dice{ 3, 6, 0 };
    DiceExpr wis_dice{ 3, 6, 0 };
    DiceExpr cha_dice{ 3, 6, 0 };

    // Weapon display + damage
    std::string weapon_name;
    DamageInfo damage{};

    // Behaviour
    MonsterAiType ai_type{ MonsterAiType::MELEE };
    bool is_levitating{ false };
    bool can_swim{ false };

    // Spawn table
    int base_weight{ 10 };
    int level_minimum{ 1 };
    int level_maximum{ 0 };
    float level_scaling{ 0.0f };
};

namespace MonsterCreator
{
    const std::unordered_map<MonsterId, MonsterParams>& get_registry();
    std::unique_ptr<Creature> create(Vector2D pos, MonsterId id, GameContext& ctx);
    std::unique_ptr<Creature> create_from_params(Vector2D pos, const MonsterParams& params, GameContext& ctx);
}
