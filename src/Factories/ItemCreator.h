#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>
#include <span>

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"  // For Pickable::PickableType
#include "../Items/MagicalItemEffects.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Systems/TargetMode.h"

struct Vector2D;
struct GameContext;
enum class ItemId;
enum class ItemClass;

// Use Pickable::PickableType - single source of truth
using PickableType = Pickable::PickableType;

// Unified compositional item parameters - supports any combination of properties
struct ItemParams
{
    // Display & Classification
    int symbol{ 0 };
    std::string_view name{ "" };
    int color{ 0 };
    ItemClass itemClass{ ItemClass::UNKNOWN };
    int value{ 0 };
    PickableType pickable_type{ PickableType::WEAPON };

    // Gameplay properties (use what you need, rest default to 0/NONE/false)

    // Consumable value: heal amount (HEAL) or buff value (ADD_BUFF)
    int consumable_amount{ 0 };

    // Ranged effects (scrolls: lightning, fireball)
    int range{ 0 };
    int damage{ 0 };

    // Confusion
    int confuse_turns{ 0 };

    // Duration effects (potions, buffs)
    int duration{ 0 };

    // Magical effects (rings, helms, protection items)
    MagicalEffect effect{ MagicalEffect::NONE };
    int effect_bonus{ 0 };

    // Stat modifications (amulets, gauntlets, girdles)
    int str_bonus{ 0 };
    int dex_bonus{ 0 };
    int con_bonus{ 0 };
    int int_bonus{ 0 };
    int wis_bonus{ 0 };
    int cha_bonus{ 0 };
    bool is_set_mode{ false };  // If true, non-zero bonuses SET stats instead of adding

    // Food
    int nutrition_value{ 0 };

    // Treasure
    int gold_amount{ 0 };

    // Armor
    int ac_bonus{ 0 };              // AD&D 2e AC bonus (negative = better AC)

    // Weapon mechanical properties (all weapons in registry should set these)
    bool ranged{ false };
    HandRequirement hand_requirement{ HandRequirement::ONE_HANDED };
    WeaponSize weapon_size{ WeaponSize::MEDIUM };

    // Consumable effect (potions and simple scrolls)
    ConsumableEffect consumable_effect{ ConsumableEffect::NONE };
    BuffType consumable_buff_type{ BuffType::INVISIBILITY }; // for ADD_BUFF effect

    // Targeted scroll properties
    TargetMode target_mode{ TargetMode::AUTO_NEAREST };
    ScrollAnimation scroll_animation{ ScrollAnimation::NONE };

    // Spawn / ItemFactory integration
    int base_weight{ 0 };              // Spawn weight (0 = not spawnable)
    int level_minimum{ 1 };            // Min dungeon level
    int level_maximum{ 0 };            // Max dungeon level (0 = no cap)
    float level_scaling{ 0.0f };       // Weight scaling per level (can be negative)
    std::string_view category{ "" };   // "potion", "weapon", "armor", etc.
};

struct ItemRegistryEntry
{
    ItemId id;
    ItemParams params;
};

enum class EnhancedItemCategory { WEAPON, ARMOR };

struct EnhancedItemSpawnRule
{
    std::span<const ItemId> item_pool;
    EnhancedItemCategory enhancement_category;
    int base_weight;
    int level_minimum;
    int level_maximum;   // 0 = no cap
    float level_scaling;
    std::string_view category;
};

class ItemCreator
{
public:

public:
    // Registry access
    static const ItemParams& get_params(ItemId itemId);
    static const std::unordered_map<ItemId, ItemParams>& get_all_params();

    // Single source of truth - data-driven item creation
    static std::unique_ptr<Item> create(ItemId itemId, Vector2D pos);
    static std::unique_ptr<Item> create_with_gold_amount(Vector2D pos, int goldAmount);
    static std::unique_ptr<Item> create_enhanced_weapon(ItemId weaponId, Vector2D pos, int enhancementLevel);
    static std::unique_ptr<Item> create_with_enhancement(ItemId itemId, Vector2D pos, PrefixType prefix, SuffixType suffix);

public:
    // Special item creation functions
    static std::unique_ptr<Item> create_gold_pile(Vector2D pos, GameContext& ctx);

    // Random generation with single source
    static std::unique_ptr<Item> create_random_of_category(std::string_view category, Vector2D pos, GameContext& ctx, int dungeonLevel);

    // Enhancement utility functions
    static int calculate_enhancement_chance(int dungeonLevel);
    static int determine_enhancement_level(GameContext& ctx, int dungeonLevel);
    static int calculate_enhanced_value(int baseValue, int enhancementLevel);
};
