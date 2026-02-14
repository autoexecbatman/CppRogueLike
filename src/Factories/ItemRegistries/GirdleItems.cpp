// file: GirdleItems.cpp
#include "GirdleItems.h"
#include "../../Items/ItemClassification.h"
#include "../../Colors/Colors.h"
#include "../../Renderer/TileId.h"

namespace
{
    const ItemRegistryEntry entries[] =
    {
        {ItemId::GIRDLE_OF_HILL_GIANT_STRENGTH, {
            .symbol = TILE_GIRDLE,
            .name = "girdle of hill giant strength",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::GIRDLE,
            .value = 5000,
            .pickable_type = PickableType::GIRDLE,
            .str_bonus = 19,
            .is_set_mode = true,
            .base_weight = 1,
            .level_minimum = 6,
            .level_scaling = 0.5f,
            .category = "girdle"}},

        {ItemId::GIRDLE_OF_STONE_GIANT_STRENGTH, {
            .symbol = TILE_GIRDLE,
            .name = "girdle of stone giant strength",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::GIRDLE,
            .value = 6000,
            .pickable_type = PickableType::GIRDLE,
            .str_bonus = 20,
            .is_set_mode = true}},

        {ItemId::GIRDLE_OF_FROST_GIANT_STRENGTH, {
            .symbol = TILE_GIRDLE,
            .name = "girdle of frost giant strength",
            .color = CYAN_BLACK_PAIR,
            .itemClass = ItemClass::GIRDLE,
            .value = 8000,
            .pickable_type = PickableType::GIRDLE,
            .str_bonus = 21,
            .is_set_mode = true,
            .base_weight = 1,
            .level_minimum = 8,
            .level_scaling = 0.6f,
            .category = "girdle"}},

        {ItemId::GIRDLE_OF_FIRE_GIANT_STRENGTH, {
            .symbol = TILE_GIRDLE,
            .name = "girdle of fire giant strength",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::GIRDLE,
            .value = 10000,
            .pickable_type = PickableType::GIRDLE,
            .str_bonus = 22,
            .is_set_mode = true}},

        {ItemId::GIRDLE_OF_CLOUD_GIANT_STRENGTH, {
            .symbol = TILE_GIRDLE,
            .name = "girdle of cloud giant strength",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::GIRDLE,
            .value = 12000,
            .pickable_type = PickableType::GIRDLE,
            .str_bonus = 23,
            .is_set_mode = true}},

        {ItemId::GIRDLE_OF_STORM_GIANT_STRENGTH, {
            .symbol = TILE_GIRDLE,
            .name = "girdle of storm giant strength",
            .color = YELLOW_BLACK_PAIR,
            .itemClass = ItemClass::GIRDLE,
            .value = 15000,
            .pickable_type = PickableType::GIRDLE,
            .str_bonus = 24,
            .is_set_mode = true}},
    };
}

std::span<const ItemRegistryEntry> get_girdle_items()
{
    return entries;
}
