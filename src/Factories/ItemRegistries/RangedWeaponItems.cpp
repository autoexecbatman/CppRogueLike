// file: RangedWeaponItems.cpp
#include "RangedWeaponItems.h"
#include "../../Colors/Colors.h"
#include "../../Items/ItemClassification.h"

namespace
{
const ItemRegistryEntry entries[] = {
	{ ItemId::SLING, { .name = "sling", .color = BROWN_BLACK_PAIR, .itemClass = ItemClass::BOW, .value = 1, .pickable_type = PickableType::WEAPON, .ranged = true, .hand_requirement = HandRequirement::ONE_HANDED, .weapon_size = WeaponSize::SMALL } },

	{ ItemId::SHORT_BOW, { .name = "short bow", .color = BROWN_BLACK_PAIR, .itemClass = ItemClass::BOW, .value = 30, .pickable_type = PickableType::WEAPON, .ranged = true, .hand_requirement = HandRequirement::TWO_HANDED, .weapon_size = WeaponSize::MEDIUM } },

	{ ItemId::LONG_BOW, { .name = "longbow", .color = WHITE_BLUE_PAIR, .itemClass = ItemClass::BOW, .value = 75, .pickable_type = PickableType::WEAPON, .ranged = true, .hand_requirement = HandRequirement::TWO_HANDED, .weapon_size = WeaponSize::LARGE, .base_weight = 12, .level_minimum = 3, .level_scaling = 0.3f, .category = "weapon" } },

	{ ItemId::COMPOSITE_BOW, { .name = "composite bow", .color = YELLOW_BLACK_PAIR, .itemClass = ItemClass::BOW, .value = 100, .pickable_type = PickableType::WEAPON, .ranged = true, .hand_requirement = HandRequirement::TWO_HANDED, .weapon_size = WeaponSize::LARGE } },

	{ ItemId::LIGHT_CROSSBOW, { .name = "light crossbow", .color = WHITE_BLACK_PAIR, .itemClass = ItemClass::CROSSBOW, .value = 35, .pickable_type = PickableType::WEAPON, .ranged = true, .hand_requirement = HandRequirement::TWO_HANDED, .weapon_size = WeaponSize::MEDIUM } },

	{ ItemId::HEAVY_CROSSBOW, { .name = "heavy crossbow", .color = WHITE_BLACK_PAIR, .itemClass = ItemClass::CROSSBOW, .value = 50, .pickable_type = PickableType::WEAPON, .ranged = true, .hand_requirement = HandRequirement::TWO_HANDED, .weapon_size = WeaponSize::LARGE } },

	{ ItemId::MEDIUM_SHIELD, { .name = "shield", .color = WHITE_BLACK_PAIR, .itemClass = ItemClass::SHIELD, .value = 10, .pickable_type = PickableType::SHIELD } },
};
}

std::span<const ItemRegistryEntry> get_ranged_weapon_items()
{
	return entries;
}
