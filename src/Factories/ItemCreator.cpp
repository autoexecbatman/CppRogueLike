#include "ItemCreator.h"
#include "../ActorTypes/Teleporter.h"
#include "../Actor/Pickable.h"
#include "../Items/Armor.h"
#include "../Items/Jewelry.h"
#include "../Items/MagicalItemEffects.h"
#include "../Colors/Colors.h"
#include "../Items/Weapons.h"
#include "../ActorTypes/Gold.h"
#include "../Items/Food.h"
#include "../Items/Amulet.h"
#include "../Random/RandomDice.h"
#include "../Core/GameContext.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Systems/BuffSystem.h"
#include "ItemRegistries/PotionItems.h"
#include "ItemRegistries/ScrollItems.h"
#include "ItemRegistries/MeleeWeaponItems.h"
#include "ItemRegistries/RangedWeaponItems.h"
#include "ItemRegistries/ArmorItems.h"
#include "ItemRegistries/HelmItems.h"
#include "ItemRegistries/RingItems.h"
#include "ItemRegistries/JewelryItems.h"
#include "ItemRegistries/GirdleItems.h"
#include "ItemRegistries/FoodItems.h"
#include <unordered_map>
#include <string_view>
#include <stdexcept>
#include <span>

namespace
{
    const std::unordered_map<ItemId, ItemParams> ItemRegistry = []()
    {
        std::unordered_map<ItemId, ItemParams> map;

        auto insert_all = [&map](std::span<const ItemRegistryEntry> entries)
        {
            for (const auto& e : entries)
                map.emplace(e.id, e.params);
        };

        insert_all(get_potion_items());
        insert_all(get_scroll_items());
        insert_all(get_melee_weapon_items());
        insert_all(get_ranged_weapon_items());
        insert_all(get_armor_items());
        insert_all(get_helm_items());
        insert_all(get_ring_items());
        insert_all(get_jewelry_items());
        insert_all(get_girdle_items());
        insert_all(get_food_items());

        return map;
    }();

    template<typename T>
    std::unique_ptr<T> create_stat_item(const ItemParams& p)
    {
        auto item = std::make_unique<T>();
        item->str_bonus = p.str_bonus;
        item->dex_bonus = p.dex_bonus;
        item->con_bonus = p.con_bonus;
        item->int_bonus = p.int_bonus;
        item->wis_bonus = p.wis_bonus;
        item->cha_bonus = p.cha_bonus;
        item->effect = p.effect;
        item->bonus = p.effect_bonus;
        item->is_set_mode = p.is_set_mode;
        return item;
    }

    std::unique_ptr<Pickable> create_pickable_from_blueprint(ItemId itemId, const ItemParams& params)
    {
        switch (params.pickable_type)
        {
            case PickableType::CONSUMABLE:
                return std::make_unique<Consumable>(
                    params.consumable_effect,
                    params.consumable_amount,
                    params.duration,
                    params.consumable_buff_type,
                    params.is_set_mode
                );

            case PickableType::TARGETED_SCROLL:
                return std::make_unique<TargetedScroll>(
                    params.target_mode,
                    params.scroll_animation,
                    params.range,
                    params.damage,
                    params.confuse_turns
                );

            case PickableType::TELEPORTER:
                return std::make_unique<Teleporter>();

            case PickableType::WEAPON:
                return std::make_unique<Weapon>(params.ranged, params.hand_requirement, params.weapon_size);

            case PickableType::SHIELD:
                return std::make_unique<Shield>();

            case PickableType::ARMOR:
                return std::make_unique<Armor>(params.ac_bonus);

            case PickableType::QUEST_ITEM:
                return std::make_unique<Amulet>();

            case PickableType::MAGICAL_HELM:
                return std::make_unique<MagicalHelm>(params.effect, params.effect_bonus);

            case PickableType::MAGICAL_RING:
                return std::make_unique<MagicalRing>(params.effect, params.effect_bonus);

            case PickableType::JEWELRY_AMULET:
                return create_stat_item<JewelryAmulet>(params);

            case PickableType::GAUNTLETS:
                return create_stat_item<Gauntlets>(params);

            case PickableType::GIRDLE:
                return create_stat_item<Girdle>(params);

            case PickableType::FOOD:
                return std::make_unique<Food>(params.nutrition_value);

            case PickableType::GOLD:
                return nullptr;

            default:
                throw std::runtime_error("Unknown pickable type: " + std::to_string(static_cast<int>(params.pickable_type)));
        }
    }

    std::unique_ptr<Item> create_from_blueprint(Vector2D pos, ItemId itemId)
    {
        const auto& params = ItemRegistry.at(itemId);
        auto item = std::make_unique<Item>(pos, ActorData{params.symbol, std::string{params.name}, params.color});
        item->pickable = create_pickable_from_blueprint(itemId, params);
        item->itemId = itemId;
        item->itemClass = params.itemClass;
        item->set_value(params.value);
        item->base_value = params.value;
        return item;
    }
}

const ItemParams& ItemCreator::get_params(ItemId itemId)
{
    return ItemRegistry.at(itemId);
}

const std::unordered_map<ItemId, ItemParams>& ItemCreator::get_all_params()
{
    return ItemRegistry;
}

std::unique_ptr<Item> ItemCreator::create(ItemId itemId, Vector2D pos)
{
    // Validate ItemId exists in registry
    if (ItemRegistry.find(itemId) == ItemRegistry.end())
    {
        throw std::invalid_argument("ItemId not found in registry: " + std::to_string(static_cast<int>(itemId)));
    }
    return create_from_blueprint(pos, itemId);
}

std::unique_ptr<Item> ItemCreator::create_with_gold_amount(Vector2D pos, int goldAmount)
{
    const auto& params = ItemRegistry.at(ItemId::GOLD);
    auto item = std::make_unique<Item>(pos, ActorData{params.symbol, std::string{params.name}, params.color});
    item->pickable = std::make_unique<Gold>(goldAmount);
    item->itemId = ItemId::GOLD;
    item->itemClass = params.itemClass;
    item->set_value(goldAmount);
    item->base_value = goldAmount;
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_weapon(ItemId weaponId, Vector2D pos, int enhancementLevel)
{
    auto weapon = create(weaponId, pos);
    weapon->set_value(calculate_enhanced_value(weapon->base_value, enhancementLevel));
    weapon->actorData.name = "+" + std::to_string(enhancementLevel) + " " + weapon->actorData.name;
    weapon->actorData.color = WHITE_GREEN_PAIR;  // Enhanced items have green color
    return weapon;
}

std::unique_ptr<Item> ItemCreator::create_with_enhancement(ItemId itemId, Vector2D pos, PrefixType prefix, SuffixType suffix)
{
    auto item = create(itemId, pos);
    ItemEnhancement enhancement(prefix, suffix);
    item->apply_enhancement(enhancement);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_gold_pile(Vector2D pos, GameContext& ctx)
{
    const int goldAmount = ctx.dice->roll(5, 20);
    return create_with_gold_amount(pos, goldAmount);
}

std::unique_ptr<Item> ItemCreator::create_random_weapon(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    static const std::vector<ItemId> weapons = {
        ItemId::DAGGER, ItemId::SHORT_SWORD, ItemId::LONG_SWORD, ItemId::GREAT_SWORD,
        ItemId::BATTLE_AXE, ItemId::GREAT_AXE, ItemId::WAR_HAMMER, ItemId::MACE, ItemId::STAFF, ItemId::LONG_BOW
    };
    const ItemId weaponId = weapons[ctx.dice->roll(0, static_cast<int>(weapons.size()) - 1)];
    return create(weaponId, pos);
}

std::unique_ptr<Item> ItemCreator::create_random_armor(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    // Only include armor types that are actually implemented in the factory
    static const std::vector<ItemId> armor = {
        ItemId::LEATHER_ARMOR,
        ItemId::CHAIN_MAIL,
        ItemId::PLATE_MAIL
    };
    const ItemId armorId = armor[ctx.dice->roll(0, static_cast<int>(armor.size()) - 1)];
    return create(armorId, pos);
}

// TODO: same as scrolls...
std::unique_ptr<Item> ItemCreator::create_random_potion(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    static const std::vector<ItemId> potions = {
        ItemId::HEALTH_POTION, ItemId::MANA_POTION, ItemId::INVISIBILITY_POTION
    };
    const ItemId potionId = potions[ctx.dice->roll(0, static_cast<int>(potions.size()) - 1)];
    return create(potionId, pos);
}

// TODO: can't we query? why always add new scrolls?
std::unique_ptr<Item> ItemCreator::create_random_scroll(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    static const std::vector<ItemId> scrolls = {
        ItemId::SCROLL_LIGHTNING, ItemId::SCROLL_FIREBALL, ItemId::SCROLL_CONFUSION, ItemId::SCROLL_TELEPORT
    };
    const ItemId scrollId = scrolls[ctx.dice->roll(0, static_cast<int>(scrolls.size()) - 1)];
    return create(scrollId, pos);
}

std::unique_ptr<Item> ItemCreator::create_weapon_with_enhancement_chance(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    auto weapon = create_random_weapon(pos, ctx, dungeonLevel);
    const int enhancementChance = calculate_enhancement_chance(dungeonLevel);
    if (ctx.dice->roll(1, 100) <= enhancementChance)
    {
        const int enhancementLevel = determine_enhancement_level(ctx, dungeonLevel);
        weapon->set_value(calculate_enhanced_value(weapon->base_value, enhancementLevel));
        weapon->actorData.name = "+" + std::to_string(enhancementLevel) + " " + weapon->actorData.name;
    }
    return weapon;
}

int ItemCreator::calculate_enhancement_chance(int dungeonLevel)
{
    return std::min(2 + (dungeonLevel * 3), 35);
}

int ItemCreator::determine_enhancement_level(GameContext& ctx, int dungeonLevel)
{
    const int roll = ctx.dice->roll(1, 100);
    if (dungeonLevel >= 10 && roll <= 5) return 3;
    if (dungeonLevel >= 5 && roll <= 20) return 2;
    return 1;
}

int ItemCreator::calculate_enhanced_value(int baseValue, int enhancementLevel)
{
    return baseValue * (1 + enhancementLevel);
}
