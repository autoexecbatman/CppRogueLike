// file: EnhancedWeaponRules.cpp
#include "EnhancedWeaponRules.h"
#include "../../Items/ItemClassification.h"

namespace
{
    const ItemId light_weapons[] =
    {
        ItemId::DAGGER,
        ItemId::SHORT_SWORD,
        ItemId::HAND_AXE,
        ItemId::CLUB,
        ItemId::SLING,
        ItemId::SHORT_BOW,
    };

    const ItemId heavy_weapons[] =
    {
        ItemId::LONG_SWORD,
        ItemId::BASTARD_SWORD,
        ItemId::SCIMITAR,
        ItemId::RAPIER,
        ItemId::GREAT_SWORD,
        ItemId::TWO_HANDED_SWORD,
        ItemId::BATTLE_AXE,
        ItemId::GREAT_AXE,
        ItemId::WAR_HAMMER,
        ItemId::MACE,
        ItemId::MORNING_STAR,
        ItemId::FLAIL,
        ItemId::STAFF,
        ItemId::QUARTERSTAFF,
        ItemId::LONG_BOW,
        ItemId::COMPOSITE_BOW,
        ItemId::LIGHT_CROSSBOW,
        ItemId::HEAVY_CROSSBOW,
    };

    const EnhancedItemSpawnRule entries[] =
    {
        {light_weapons, EnhancedItemCategory::WEAPON,
         8, 1, 4, 0.2f, "enhanced_weapon"},

        {heavy_weapons, EnhancedItemCategory::WEAPON,
         5, 3, 0, 0.4f, "enhanced_weapon"},
    };
}

std::span<const EnhancedItemSpawnRule> get_enhanced_weapon_rules()
{
    return entries;
}
