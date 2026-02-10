// file: JewelryItems.cpp
// Amulets, gauntlets, boots, cloaks
#include "JewelryItems.h"
#include "../../Items/ItemClassification.h"
#include "../../Colors/Colors.h"

namespace
{
    const ItemRegistryEntry entries[] =
    {
        // === AMULETS ===
        {ItemId::AMULET_OF_HEALTH, {
            .symbol = '"',
            .name = "amulet of health",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::AMULET,
            .value = 200,
            .pickable_type = PickableType::JEWELRY_AMULET,
            .con_bonus = 1}},

        {ItemId::AMULET_OF_WISDOM, {
            .symbol = '"',
            .name = "amulet of wisdom",
            .color = BLUE_BLACK_PAIR,
            .itemClass = ItemClass::AMULET,
            .value = 200,
            .pickable_type = PickableType::JEWELRY_AMULET,
            .wis_bonus = 1}},

        {ItemId::AMULET_OF_PROTECTION, {
            .symbol = '"',
            .name = "amulet of protection",
            .color = CYAN_BLACK_PAIR,
            .itemClass = ItemClass::AMULET,
            .value = 150,
            .pickable_type = PickableType::JEWELRY_AMULET}},

        {ItemId::AMULET_OF_OGRE_POWER, {
            .symbol = '"',
            .name = "amulet of ogre power",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::AMULET,
            .value = 3000,
            .pickable_type = PickableType::JEWELRY_AMULET,
            .effect = MagicalEffect::PROTECTION,
            .effect_bonus = 1,
            .str_bonus = 18,
            .is_set_mode = true}},

        {ItemId::AMULET_OF_YENDOR, {
            .symbol = '"',
            .name = "Amulet of Yendor",
            .color = YELLOW_BLACK_PAIR,
            .itemClass = ItemClass::QUEST_ITEM,
            .value = 50000,
            .pickable_type = PickableType::QUEST_ITEM}},

        // === GAUNTLETS ===
        {ItemId::GAUNTLETS_OF_OGRE_POWER, {
            .symbol = '[',
            .name = "gauntlets of ogre power",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::GAUNTLETS,
            .value = 3000,
            .pickable_type = PickableType::GAUNTLETS,
            .str_bonus = 18,
            .is_set_mode = true,
            .base_weight = 1,
            .level_minimum = 5,
            .level_scaling = 0.4f,
            .category = "gauntlets"}},

        {ItemId::GAUNTLETS_OF_DEXTERITY, {
            .symbol = '[',
            .name = "gauntlets of dexterity",
            .color = GREEN_BLACK_PAIR,
            .itemClass = ItemClass::GAUNTLETS,
            .value = 2000,
            .pickable_type = PickableType::GAUNTLETS,
            .dex_bonus = 2,
            .base_weight = 1,
            .level_minimum = 4,
            .level_scaling = 0.3f,
            .category = "gauntlets"}},

        {ItemId::GAUNTLETS_OF_SWIMMING_AND_CLIMBING, {
            .symbol = '[',
            .name = "gauntlets of swimming and climbing",
            .color = BLUE_BLACK_PAIR,
            .itemClass = ItemClass::GAUNTLETS,
            .value = 3000,
            .pickable_type = PickableType::GAUNTLETS,
            .str_bonus = 2}},

        {ItemId::GAUNTLETS_OF_FUMBLING, {
            .symbol = '[',
            .name = "gauntlets of fumbling",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::GAUNTLETS,
            .value = 1,
            .pickable_type = PickableType::GAUNTLETS,
            .dex_bonus = -2}},

        // === BOOTS ===
        {ItemId::BOOTS_OF_SPEED, {
            .symbol = '[',
            .name = "boots of speed",
            .color = GREEN_BLACK_PAIR,
            .itemClass = ItemClass::GAUNTLETS,
            .value = 5000,
            .pickable_type = PickableType::GAUNTLETS,
            .dex_bonus = 2}},

        {ItemId::BOOTS_OF_ELVENKIND, {
            .symbol = '[',
            .name = "boots of elvenkind",
            .color = GREEN_BLACK_PAIR,
            .itemClass = ItemClass::GAUNTLETS,
            .value = 3000,
            .pickable_type = PickableType::GAUNTLETS,
            .dex_bonus = 1}},

// === CLOAKS ===
        {ItemId::CLOAK_OF_PROTECTION, {
            .symbol = '[',
            .name = "cloak of protection",
            .color = CYAN_BLACK_PAIR,
            .itemClass = ItemClass::GAUNTLETS,
            .value = 3000,
            .pickable_type = PickableType::GAUNTLETS,
            .effect = MagicalEffect::PROTECTION,
            .effect_bonus = 1}},

        {ItemId::CLOAK_OF_DISPLACEMENT, {
            .symbol = '[',
            .name = "cloak of displacement",
            .color = MAGENTA_BLACK_PAIR,
            .itemClass = ItemClass::GAUNTLETS,
            .value = 5000,
            .pickable_type = PickableType::GAUNTLETS}},

        {ItemId::CLOAK_OF_ELVENKIND, {
            .symbol = '[',
            .name = "cloak of elvenkind",
            .color = GREEN_BLACK_PAIR,
            .itemClass = ItemClass::GAUNTLETS,
            .value = 4000,
            .pickable_type = PickableType::GAUNTLETS,
            .dex_bonus = 1}},
    };
}

std::span<const ItemRegistryEntry> get_jewelry_items()
{
    return entries;
}
