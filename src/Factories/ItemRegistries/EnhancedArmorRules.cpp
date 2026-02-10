// file: EnhancedArmorRules.cpp
#include "EnhancedArmorRules.h"
#include "../../Items/ItemClassification.h"

namespace
{
    const ItemId light_armor[] =
    {
        ItemId::PADDED_ARMOR,
        ItemId::LEATHER_ARMOR,
        ItemId::STUDDED_LEATHER,
        ItemId::HIDE_ARMOR,
        ItemId::RING_MAIL,
    };

    const ItemId heavy_armor[] =
    {
        ItemId::SCALE_MAIL,
        ItemId::CHAIN_MAIL,
        ItemId::BRIGANDINE,
        ItemId::SPLINT_MAIL,
        ItemId::BANDED_MAIL,
        ItemId::PLATE_MAIL,
        ItemId::FIELD_PLATE,
        ItemId::FULL_PLATE,
    };

    const EnhancedItemSpawnRule entries[] =
    {
        {light_armor, EnhancedItemCategory::ARMOR,
         6, 1, 5, 0.2f, "enhanced_armor"},

        {heavy_armor, EnhancedItemCategory::ARMOR,
         4, 4, 0, 0.4f, "enhanced_armor"},
    };
}

std::span<const EnhancedItemSpawnRule> get_enhanced_armor_rules()
{
    return entries;
}
