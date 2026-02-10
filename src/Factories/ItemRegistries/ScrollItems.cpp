// file: ScrollItems.cpp
#include "ScrollItems.h"
#include "../../Items/ItemClassification.h"
#include "../../Colors/Colors.h"

namespace
{
    const ItemRegistryEntry entries[] =
    {
        {ItemId::SCROLL_LIGHTNING, {
            .symbol = '?',
            .name = "scroll of lightning bolt",
            .color = WHITE_BLUE_PAIR,
            .itemClass = ItemClass::SCROLL,
            .value = 150,
            .pickable_type = PickableType::TARGETED_SCROLL,
            .range = 5,
            .damage = 20,
            .target_mode = TargetMode::AUTO_NEAREST,
            .scroll_animation = ScrollAnimation::LIGHTNING,
            .base_weight = 20,
            .level_minimum = 2,
            .level_scaling = 0.2f,
            .category = "scroll"}},

        {ItemId::SCROLL_FIREBALL, {
            .symbol = '?',
            .name = "scroll of fireball",
            .color = RED_YELLOW_PAIR,
            .itemClass = ItemClass::SCROLL,
            .value = 100,
            .pickable_type = PickableType::TARGETED_SCROLL,
            .range = 3,
            .damage = 12,
            .target_mode = TargetMode::PICK_TILE_AOE,
            .scroll_animation = ScrollAnimation::EXPLOSION,
            .base_weight = 15,
            .level_minimum = 3,
            .level_scaling = 0.3f,
            .category = "scroll"}},

        {ItemId::SCROLL_CONFUSION, {
            .symbol = '?',
            .name = "scroll of confusion",
            .color = WHITE_GREEN_PAIR,
            .itemClass = ItemClass::SCROLL,
            .value = 120,
            .pickable_type = PickableType::TARGETED_SCROLL,
            .range = 10,
            .confuse_turns = 8,
            .target_mode = TargetMode::PICK_TILE_SINGLE,
            .scroll_animation = ScrollAnimation::NONE,
            .base_weight = 15,
            .level_minimum = 2,
            .level_scaling = 0.2f,
            .category = "scroll"}},

        {ItemId::SCROLL_TELEPORT, {
            .symbol = '?',
            .name = "scroll of teleportation",
            .color = MAGENTA_BLACK_PAIR,
            .itemClass = ItemClass::SCROLL,
            .value = 200,
            .pickable_type = PickableType::TELEPORTER}},
    };
}

std::span<const ItemRegistryEntry> get_scroll_items()
{
    return entries;
}
