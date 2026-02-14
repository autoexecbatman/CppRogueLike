// file: RingItems.cpp
#include "RingItems.h"
#include "../../Items/ItemClassification.h"
#include "../../Colors/Colors.h"
#include "../../Renderer/TileId.h"

namespace
{
    const ItemRegistryEntry entries[] =
    {
        {ItemId::RING_OF_PROTECTION_PLUS_1, {
            .symbol = TILE_RING,
            .name = "ring of protection +1",
            .color = CYAN_BLACK_PAIR,
            .itemClass = ItemClass::RING,
            .value = 2000,
            .pickable_type = PickableType::MAGICAL_RING,
            .effect = MagicalEffect::PROTECTION,
            .effect_bonus = 1,
            .base_weight = 2,
            .level_minimum = 3,
            .level_scaling = 0.3f,
            .category = "magical_ring"}},

        {ItemId::RING_OF_PROTECTION_PLUS_2, {
            .symbol = TILE_RING,
            .name = "ring of protection +2",
            .color = CYAN_BLACK_PAIR,
            .itemClass = ItemClass::RING,
            .value = 5000,
            .pickable_type = PickableType::MAGICAL_RING,
            .effect = MagicalEffect::PROTECTION,
            .effect_bonus = 2,
            .base_weight = 1,
            .level_minimum = 6,
            .level_scaling = 0.4f,
            .category = "magical_ring"}},

        {ItemId::RING_OF_FREE_ACTION, {
            .symbol = TILE_RING,
            .name = "ring of free action",
            .color = GREEN_BLACK_PAIR,
            .itemClass = ItemClass::RING,
            .value = 4000,
            .pickable_type = PickableType::MAGICAL_RING,
            .effect = MagicalEffect::FREE_ACTION,
            .base_weight = 1,
            .level_minimum = 4,
            .level_scaling = 0.3f,
            .category = "magical_ring"}},

        {ItemId::RING_OF_REGENERATION, {
            .symbol = TILE_RING,
            .name = "ring of regeneration",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::RING,
            .value = 8000,
            .pickable_type = PickableType::MAGICAL_RING,
            .effect = MagicalEffect::REGENERATION,
            .base_weight = 1,
            .level_minimum = 7,
            .level_scaling = 0.5f,
            .category = "magical_ring"}},

        {ItemId::RING_OF_INVISIBILITY, {
            .symbol = TILE_RING,
            .name = "ring of invisibility",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::RING,
            .value = 6000,
            .pickable_type = PickableType::MAGICAL_RING,
            .effect = MagicalEffect::INVISIBILITY,
            .base_weight = 1,
            .level_minimum = 6,
            .level_scaling = 0.4f,
            .category = "magical_ring"}},

        {ItemId::RING_OF_FIRE_RESISTANCE, {
            .symbol = TILE_RING,
            .name = "ring of fire resistance",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::RING,
            .value = 5000,
            .pickable_type = PickableType::MAGICAL_RING,
            .effect = MagicalEffect::FIRE_RESISTANCE}},

        {ItemId::RING_OF_COLD_RESISTANCE, {
            .symbol = TILE_RING,
            .name = "ring of cold resistance",
            .color = CYAN_BLACK_PAIR,
            .itemClass = ItemClass::RING,
            .value = 5000,
            .pickable_type = PickableType::MAGICAL_RING,
            .effect = MagicalEffect::COLD_RESISTANCE}},

        {ItemId::RING_OF_SPELL_STORING, {
            .symbol = TILE_RING,
            .name = "ring of spell storing",
            .color = MAGENTA_BLACK_PAIR,
            .itemClass = ItemClass::RING,
            .value = 10000,
            .pickable_type = PickableType::MAGICAL_RING,
            .effect = MagicalEffect::SPELL_STORING}},
    };
}

std::span<const ItemRegistryEntry> get_ring_items()
{
    return entries;
}
