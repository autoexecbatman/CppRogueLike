// file: ScrollItems.cpp
#include <span>

#include "../../Actor/Pickable.h"
#include "../../Colors/Colors.h"
#include "../../Items/ItemClassification.h"
#include "../../Systems/BuffType.h"
#include "../../Systems/TargetMode.h"
#include "../ItemCreator.h"
#include "ScrollItems.h"

namespace
{
const ItemRegistryEntry entries[] = {
	{ ItemId::SCROLL_LIGHTNING, { .name = "scroll of lightning bolt", .color = WHITE_BLUE_PAIR, .itemClass = ItemClass::SCROLL, .value = 150, .pickable_type = PickableType::TARGETED_SCROLL, .range = 5, .damage = 20, .target_mode = TargetMode::AUTO_NEAREST, .scroll_animation = ScrollAnimation::LIGHTNING, .base_weight = 20, .level_minimum = 2, .level_scaling = 0.2f, .category = "scroll" } },

	{ ItemId::SCROLL_FIREBALL, { .name = "scroll of fireball", .color = RED_YELLOW_PAIR, .itemClass = ItemClass::SCROLL, .value = 100, .pickable_type = PickableType::TARGETED_SCROLL, .range = 3, .damage = 12, .target_mode = TargetMode::PICK_TILE_AOE, .scroll_animation = ScrollAnimation::EXPLOSION, .base_weight = 15, .level_minimum = 3, .level_scaling = 0.3f, .category = "scroll" } },

	{ ItemId::SCROLL_CONFUSION, { .name = "scroll of confusion", .color = WHITE_GREEN_PAIR, .itemClass = ItemClass::SCROLL, .value = 120, .pickable_type = PickableType::TARGETED_SCROLL, .range = 10, .confuse_turns = 8, .target_mode = TargetMode::PICK_TILE_SINGLE, .scroll_animation = ScrollAnimation::NONE, .base_weight = 15, .level_minimum = 2, .level_scaling = 0.2f, .category = "scroll" } },

	{ ItemId::SCROLL_TELEPORT, { .name = "scroll of teleportation", .color = MAGENTA_BLACK_PAIR, .itemClass = ItemClass::SCROLL, .value = 200, .pickable_type = PickableType::TELEPORTER } },

	{ ItemId::SCROLL_SLEEP, { .name = "scroll of sleep", .color = CYAN_BLACK_PAIR, .itemClass = ItemClass::SCROLL, .value = 100, .pickable_type = PickableType::TARGETED_SCROLL, .duration = 5, .consumable_buff_type = BuffType::SLEEP, .target_mode = TargetMode::FOV_BUFF, .scroll_animation = ScrollAnimation::NONE, .base_weight = 20, .level_minimum = 1, .level_scaling = 0.1f, .category = "scroll" } },

	{ ItemId::SCROLL_HOLD_PERSON, { .name = "scroll of hold person", .color = WHITE_BLACK_PAIR, .itemClass = ItemClass::SCROLL, .value = 150, .pickable_type = PickableType::TARGETED_SCROLL, .duration = 8, .consumable_buff_type = BuffType::HOLD_PERSON, .target_mode = TargetMode::FOV_BUFF, .scroll_animation = ScrollAnimation::NONE, .base_weight = 15, .level_minimum = 2, .level_scaling = 0.15f, .category = "scroll" } },
};
}

std::span<const ItemRegistryEntry> get_scroll_items()
{
	return entries;
}
