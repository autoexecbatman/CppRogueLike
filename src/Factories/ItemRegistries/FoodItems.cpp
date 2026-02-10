// file: FoodItems.cpp
// Food and gold
#include "FoodItems.h"
#include "../../Items/ItemClassification.h"
#include "../../Colors/Colors.h"

namespace
{
    const ItemRegistryEntry entries[] =
    {
        {ItemId::FOOD_RATION, {
            .symbol = '%',
            .name = "ration",
            .color = WHITE_GREEN_PAIR,
            .itemClass = ItemClass::FOOD,
            .value = 10,
            .pickable_type = PickableType::FOOD,
            .nutrition_value = 300,
            .base_weight = 25,
            .level_minimum = 1,
            .level_scaling = 0.1f,
            .category = "food"}},

        {ItemId::FRUIT, {
            .symbol = '%',
            .name = "fruit",
            .color = GREEN_BLACK_PAIR,
            .itemClass = ItemClass::FOOD,
            .value = 3,
            .pickable_type = PickableType::FOOD,
            .nutrition_value = 100,
            .base_weight = 15,
            .level_minimum = 1,
            .category = "food"}},

        {ItemId::BREAD, {
            .symbol = '%',
            .name = "bread",
            .color = RED_YELLOW_PAIR,
            .itemClass = ItemClass::FOOD,
            .value = 5,
            .pickable_type = PickableType::FOOD,
            .nutrition_value = 200,
            .base_weight = 12,
            .level_minimum = 1,
            .category = "food"}},

        {ItemId::MEAT, {
            .symbol = '%',
            .name = "meat",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::FOOD,
            .value = 8,
            .pickable_type = PickableType::FOOD,
            .nutrition_value = 250,
            .base_weight = 8,
            .level_minimum = 2,
            .level_scaling = 0.1f,
            .category = "food"}},

        {ItemId::GOLD, {
            .symbol = '$',
            .name = "gold pile",
            .color = YELLOW_BLACK_PAIR,
            .itemClass = ItemClass::GOLD,
            .value = 0,
            .pickable_type = PickableType::GOLD,
            .base_weight = 25,
            .level_minimum = 1,
            .level_scaling = 0.1f,
            .category = "gold"}},
    };
}

std::span<const ItemRegistryEntry> get_food_items()
{
    return entries;
}
