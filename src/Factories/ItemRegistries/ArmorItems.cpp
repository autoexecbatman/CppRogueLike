// file: ArmorItems.cpp
#include "ArmorItems.h"
#include "../../Items/ItemClassification.h"
#include "../../Colors/Colors.h"
#include "../../Renderer/TileId.h"

namespace
{
    const ItemRegistryEntry entries[] =
    {
        // AC 8 — appears early, fades out quickly
        {ItemId::PADDED_ARMOR, {
            .symbol = TILE_ARMOR,
            .name = "padded armor",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 4,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -2,
            .base_weight = 3,
            .level_minimum = 1,
            .level_maximum = 3,
            .level_scaling = -0.5f,
            .category = "armor"}},

        // AC 8 — common early, tapers off
        {ItemId::LEATHER_ARMOR, {
            .symbol = TILE_ARMOR,
            .name = "leather armor",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 5,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -2,
            .base_weight = 2,
            .level_minimum = 1,
            .level_scaling = -0.4f,
            .category = "armor"}},

        // AC 7 — early to mid
        {ItemId::STUDDED_LEATHER, {
            .symbol = TILE_ARMOR,
            .name = "studded leather",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 20,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -3,
            .base_weight = 3,
            .level_minimum = 1,
            .level_maximum = 5,
            .level_scaling = -0.2f,
            .category = "armor"}},

        // AC 7 — early to mid, rarer than leather
        {ItemId::HIDE_ARMOR, {
            .symbol = TILE_ARMOR,
            .name = "hide armor",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 15,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -3,
            .base_weight = 2,
            .level_minimum = 1,
            .level_maximum = 5,
            .level_scaling = -0.2f,
            .category = "armor"}},

        // AC 7 — starts lvl 2, mid game
        {ItemId::RING_MAIL, {
            .symbol = TILE_ARMOR,
            .name = "ring mail",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 30,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -3,
            .base_weight = 3,
            .level_minimum = 2,
            .level_maximum = 6,
            .level_scaling = -0.2f,
            .category = "armor"}},

        // AC 6 — mid game
        {ItemId::SCALE_MAIL, {
            .symbol = TILE_ARMOR,
            .name = "scale mail",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 45,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -4,
            .base_weight = 3,
            .level_minimum = 3,
            .level_maximum = 7,
            .level_scaling = -0.1f,
            .category = "armor"}},

        // AC 5 — mid game, stays relevant longer
        {ItemId::CHAIN_MAIL, {
            .symbol = TILE_ARMOR,
            .name = "chain mail",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 75,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -5,
            .base_weight = 3,
            .level_minimum = 3,
            .level_scaling = -0.3f,
            .category = "armor"}},

        // AC 6 — mid game, comparable to scale
        {ItemId::BRIGANDINE, {
            .symbol = TILE_ARMOR,
            .name = "brigandine",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 120,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -4,
            .base_weight = 2,
            .level_minimum = 3,
            .level_maximum = 7,
            .level_scaling = -0.1f,
            .category = "armor"}},

        // AC 4 — late mid, grows with level
        {ItemId::SPLINT_MAIL, {
            .symbol = TILE_ARMOR,
            .name = "splint mail",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 80,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -6,
            .base_weight = 2,
            .level_minimum = 5,
            .level_scaling = 0.1f,
            .category = "armor"}},

        // AC 4 — late mid, grows with level
        {ItemId::BANDED_MAIL, {
            .symbol = TILE_ARMOR,
            .name = "banded mail",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 90,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -6,
            .base_weight = 2,
            .level_minimum = 5,
            .level_scaling = 0.1f,
            .category = "armor"}},

        // AC 3 — late game, uncommon
        {ItemId::PLATE_MAIL, {
            .symbol = TILE_ARMOR,
            .name = "plate mail",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 400,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -7,
            .base_weight = 1,
            .level_minimum = 5,
            .level_scaling = -0.5f,
            .category = "armor"}},

        // AC 2 — rare, deep dungeon
        {ItemId::FIELD_PLATE, {
            .symbol = TILE_ARMOR,
            .name = "field plate",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 2000,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -8,
            .base_weight = 1,
            .level_minimum = 7,
            .level_scaling = 0.2f,
            .category = "armor"}},

        // AC 1 — very rare, endgame only
        {ItemId::FULL_PLATE, {
            .symbol = TILE_ARMOR,
            .name = "full plate",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 4000,
            .pickable_type = PickableType::ARMOR,
            .ac_bonus = -9,
            .base_weight = 1,
            .level_minimum = 9,
            .level_scaling = 0.3f,
            .category = "armor"}},
    };
}

std::span<const ItemRegistryEntry> get_armor_items()
{
    return entries;
}
