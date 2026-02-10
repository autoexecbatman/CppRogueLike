// file: PotionItems.cpp
#include "PotionItems.h"
#include "../../Items/ItemClassification.h"
#include "../../Colors/Colors.h"

namespace
{
    const ItemRegistryEntry entries[] =
    {
        {ItemId::HEALTH_POTION, {
            .symbol = '!',
            .name = "health potion",
            .color = WHITE_RED_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 50,
            .pickable_type = PickableType::CONSUMABLE,
            .consumable_amount = 10,
            .consumable_effect = ConsumableEffect::HEAL,
            .base_weight = 50,
            .level_minimum = 1,
            .level_scaling = 0.2f,
            .category = "potion"}},

        {ItemId::POTION_OF_EXTRA_HEALING, {
            .symbol = '!',
            .name = "potion of extra healing",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 100,
            .pickable_type = PickableType::CONSUMABLE,
            .consumable_amount = 30,
            .consumable_effect = ConsumableEffect::HEAL}},

        {ItemId::MANA_POTION, {
            .symbol = '!',
            .name = "mana potion",
            .color = BLUE_BLACK_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 50,
            .pickable_type = PickableType::CONSUMABLE,
            .consumable_effect = ConsumableEffect::NONE}},

        {ItemId::INVISIBILITY_POTION, {
            .symbol = '!',
            .name = "invisibility potion",
            .color = CYAN_BLACK_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 150,
            .pickable_type = PickableType::CONSUMABLE,
            .duration = 30,
            .consumable_effect = ConsumableEffect::ADD_BUFF,
            .consumable_buff_type = BuffType::INVISIBILITY}},

        {ItemId::POTION_OF_GIANT_STRENGTH, {
            .symbol = '!',
            .name = "potion of giant strength",
            .color = YELLOW_BLACK_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 200,
            .pickable_type = PickableType::CONSUMABLE,
            .consumable_amount = 19,
            .duration = 50,
            .is_set_mode = true,
            .consumable_effect = ConsumableEffect::ADD_BUFF,
            .consumable_buff_type = BuffType::STRENGTH}},

{ItemId::POTION_OF_FIRE_RESISTANCE, {
            .symbol = '!',
            .name = "potion of fire resistance",
            .color = RED_YELLOW_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 150,
            .pickable_type = PickableType::CONSUMABLE,
            .consumable_effect = ConsumableEffect::NONE}},

        {ItemId::POTION_OF_COLD_RESISTANCE, {
            .symbol = '!',
            .name = "potion of cold resistance",
            .color = CYAN_BLACK_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 150,
            .pickable_type = PickableType::CONSUMABLE,
            .consumable_effect = ConsumableEffect::NONE}},

        {ItemId::POTION_OF_SPEED, {
            .symbol = '!',
            .name = "potion of speed",
            .color = GREEN_BLACK_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 200,
            .pickable_type = PickableType::CONSUMABLE,
            .consumable_effect = ConsumableEffect::NONE}},
    };
}

std::span<const ItemRegistryEntry> get_potion_items()
{
    return entries;
}
