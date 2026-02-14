// file: MeleeWeaponItems.cpp
#include "MeleeWeaponItems.h"
#include "../../Items/ItemClassification.h"
#include "../../Colors/Colors.h"
#include "../../Renderer/TileId.h"

namespace
{
    const ItemRegistryEntry entries[] =
    {
        {ItemId::DAGGER, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "dagger",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::DAGGER,
            .value = 2,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::TINY,
            .base_weight = 3,
            .level_minimum = 1,
            .level_maximum = 3,
            .level_scaling = -0.5f,
            .category = "weapon"}},

        {ItemId::SHORT_SWORD, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "short sword",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::SWORD,
            .value = 10,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::SMALL,
            .base_weight = 5,
            .level_minimum = 1,
            .level_maximum = 4,
            .level_scaling = -0.4f,
            .category = "weapon"}},

        {ItemId::LONG_SWORD, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "long sword",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::SWORD,
            .value = 15,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM,
            .base_weight = 6,
            .level_minimum = 1,
            .level_scaling = -0.2f,
            .category = "weapon"}},

        {ItemId::BASTARD_SWORD, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "bastard sword",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::SWORD,
            .value = 25,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::SCIMITAR, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "scimitar",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::SWORD,
            .value = 15,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::RAPIER, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "rapier",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::SWORD,
            .value = 15,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::GREAT_SWORD, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "greatsword",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::GREAT_SWORD,
            .value = 50,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::LARGE}},

        {ItemId::TWO_HANDED_SWORD, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "two-handed sword",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::GREAT_SWORD,
            .value = 45,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::LARGE}},

        {ItemId::HAND_AXE, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "hand axe",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::AXE,
            .value = 8,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::SMALL}},

        {ItemId::BATTLE_AXE, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "battle axe",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::AXE,
            .value = 25,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::GREAT_AXE, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "great axe",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::AXE,
            .value = 40,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::LARGE}},

        {ItemId::CLUB, {
            .symbol = TILE_TWO_HANDED,
            .name = "club",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::MACE,
            .value = 1,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::SMALL}},

        {ItemId::MACE, {
            .symbol = TILE_TWO_HANDED,
            .name = "mace",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::MACE,
            .value = 8,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::WAR_HAMMER, {
            .symbol = TILE_TWO_HANDED,
            .name = "war hammer",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::HAMMER,
            .value = 20,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::MORNING_STAR, {
            .symbol = TILE_TWO_HANDED,
            .name = "morning star",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::MACE,
            .value = 10,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::FLAIL, {
            .symbol = TILE_TWO_HANDED,
            .name = "flail",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::MACE,
            .value = 8,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::STAFF, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "staff",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::STAFF,
            .value = 6,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::LARGE,
            .base_weight = 8,
            .level_minimum = 2,
            .level_scaling = 0.1f,
            .category = "weapon"}},

        {ItemId::QUARTERSTAFF, {
            .symbol = TILE_MELEE_WEAPON,
            .name = "quarterstaff",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::STAFF,
            .value = 1,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::LARGE}},
    };
}

std::span<const ItemRegistryEntry> get_melee_weapon_items()
{
    return entries;
}
