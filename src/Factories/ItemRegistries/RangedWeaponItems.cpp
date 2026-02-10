// file: RangedWeaponItems.cpp
#include "RangedWeaponItems.h"
#include "../../Items/ItemClassification.h"
#include "../../Colors/Colors.h"

namespace
{
    const ItemRegistryEntry entries[] =
    {
        {ItemId::SLING, {
            .symbol = ')',
            .name = "sling",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::BOW,
            .value = 1,
            .pickable_type = PickableType::WEAPON,
            .ranged = true,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::SMALL}},

        {ItemId::SHORT_BOW, {
            .symbol = ')',
            .name = "short bow",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::BOW,
            .value = 30,
            .pickable_type = PickableType::WEAPON,
            .ranged = true,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::LONG_BOW, {
            .symbol = ')',
            .name = "longbow",
            .color = WHITE_BLUE_PAIR,
            .itemClass = ItemClass::BOW,
            .value = 75,
            .pickable_type = PickableType::WEAPON,
            .ranged = true,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::LARGE,
            .base_weight = 12,
            .level_minimum = 3,
            .level_scaling = 0.3f,
            .category = "weapon"}},

        {ItemId::COMPOSITE_BOW, {
            .symbol = ')',
            .name = "composite bow",
            .color = YELLOW_BLACK_PAIR,
            .itemClass = ItemClass::BOW,
            .value = 100,
            .pickable_type = PickableType::WEAPON,
            .ranged = true,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::LARGE}},

        {ItemId::LIGHT_CROSSBOW, {
            .symbol = ')',
            .name = "light crossbow",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::CROSSBOW,
            .value = 35,
            .pickable_type = PickableType::WEAPON,
            .ranged = true,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::HEAVY_CROSSBOW, {
            .symbol = ')',
            .name = "heavy crossbow",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::CROSSBOW,
            .value = 50,
            .pickable_type = PickableType::WEAPON,
            .ranged = true,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::LARGE}},

        {ItemId::MEDIUM_SHIELD, {
            .symbol = '[',
            .name = "shield",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::SHIELD,
            .value = 10,
            .pickable_type = PickableType::SHIELD}},
    };
}

std::span<const ItemRegistryEntry> get_ranged_weapon_items()
{
    return entries;
}
