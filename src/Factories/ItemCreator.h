#pragma once

#include <memory>
#include <string_view>

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
    char symbol{ '\0' };
    std::string_view name{ "" };
    int color{ 0 };
    ItemClass itemClass{ ItemClass::UNKNOWN };
    int value{ 0 };
    PickableType pickable_type{ PickableType::WEAPON };

    // Gameplay properties (use what you need, rest default to 0/NONE/false)

    // Healing
    int heal_amount{ 0 };

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
};

class ItemCreator
{
public:

private:
    // TODO: why are the not implemented? Investigate please if this is correct.
    template <typename TPickable, typename... Args>
    static std::unique_ptr<Item> base_create(Vector2D pos, ItemId itemId, Args&&... args);

    template <typename TWeapon>
    static std::unique_ptr<Item> create_enhanced(Vector2D pos, ItemId itemId, int enhancementLevel);

    template <typename TPickable, typename ConfigFunc>
    static std::unique_ptr<Item> base_create_with_config(Vector2D pos, ItemId itemId, ConfigFunc&& config);

public:
    // Registry access
    static const ItemParams& get_params(ItemId itemId);

    // Single source of truth - data-driven item creation
    static std::unique_ptr<Item> create(ItemId itemId, Vector2D pos);
    static std::unique_ptr<Item> create_with_gold_amount(Vector2D pos, int goldAmount);
    static std::unique_ptr<Item> create_enhanced_weapon(ItemId weaponId, Vector2D pos, int enhancementLevel);
    static std::unique_ptr<Item> create_with_enhancement(ItemId itemId, Vector2D pos, PrefixType prefix, SuffixType suffix);

public:
    // Special item creation functions
    static std::unique_ptr<Item> create_gold_pile(Vector2D pos, GameContext& ctx);

    // Random generation with single source
    static std::unique_ptr<Item> create_random_weapon(Vector2D pos, GameContext& ctx, int dungeonLevel = 1);
    static std::unique_ptr<Item> create_random_armor(Vector2D pos, GameContext& ctx, int dungeonLevel = 1);
    static std::unique_ptr<Item> create_random_potion(Vector2D pos, GameContext& ctx, int dungeonLevel = 1);
    static std::unique_ptr<Item> create_random_scroll(Vector2D pos, GameContext& ctx, int dungeonLevel = 1);
    static std::unique_ptr<Item> create_weapon_with_enhancement_chance(Vector2D pos, GameContext& ctx, int dungeonLevel = 1);

    // Enhancement utility functions
    static int calculate_enhancement_chance(int dungeonLevel);
    static int determine_enhancement_level(GameContext& ctx, int dungeonLevel);
    static int calculate_enhanced_value(int baseValue, int enhancementLevel);
};
