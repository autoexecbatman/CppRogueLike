#include "ItemCreator.h"
#include "../ActorTypes/Teleporter.h"
#include "../Actor/Pickable.h"
#include "../Items/Armor.h"
#include "../Items/Jewelry.h"
#include "../Items/MagicalItemEffects.h"
#include "../Colors/Colors.h"
#include "../Items/Weapons.h"
#include "../ActorTypes/Gold.h"
#include "../Items/Food.h"
#include "../Items/Amulet.h"
#include "../Random/RandomDice.h"
#include "../Core/GameContext.h"
#include "../Systems/ItemEnhancements/ItemEnhancements.h"
#include "../Systems/BuffSystem.h"
#include <unordered_map>
#include <string_view>
#include <stdexcept>

namespace
{
    const std::unordered_map<ItemId, ItemParams> ItemRegistry = {
        // === POTIONS ===

        {ItemId::HEALTH_POTION, {
            .symbol = '!',
            .name = "health potion",
            .color = WHITE_RED_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 50,
            .pickable_type = PickableType::CONSUMABLE,
            .heal_amount = 10,
            .consumable_effect = ConsumableEffect::HEAL}},

        {ItemId::POTION_OF_EXTRA_HEALING, {
            .symbol = '!',
            .name = "potion of extra healing",
            .color = RED_BLACK_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 100,
            .pickable_type = PickableType::CONSUMABLE,
            .heal_amount = 30,
            .consumable_effect = ConsumableEffect::HEAL}},

        {ItemId::MANA_POTION, { // TODO: Check Mana potion we don't actually have mana. Is it a adnd2e item?
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
            .heal_amount = 19,  // Sets STR to 19 (Hill Giant)
            .duration = 50,
            .is_set_mode = true,
            .consumable_effect = ConsumableEffect::ADD_BUFF,
            .consumable_buff_type = BuffType::STRENGTH}},

        {ItemId::POTION_OF_LEVITATION, { // TODO: Check validity of item.
            .symbol = '!', 
            .name = "potion of levitation",
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::POTION,
            .value = 150, 
            .pickable_type = PickableType::CONSUMABLE, 
            .consumable_effect = ConsumableEffect::NONE}},

        {ItemId::POTION_OF_FIRE_RESISTANCE, { // TODO: Check validity of item. No duration?
            .symbol = '!',
            .name = "potion of fire resistance",
            .color = RED_YELLOW_PAIR,
            .itemClass = ItemClass::POTION, 
            .value = 150, 
            .pickable_type = PickableType::CONSUMABLE, 
            .consumable_effect = ConsumableEffect::NONE}},

        {ItemId::POTION_OF_COLD_RESISTANCE, { // TODO: Check validity of item. Same as fire...
            .symbol = '!',
            .name = "potion of cold resistance",
            .color = CYAN_BLACK_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 150,
            .pickable_type = PickableType::CONSUMABLE,
            .consumable_effect = ConsumableEffect::NONE}},

        {ItemId::POTION_OF_SPEED, { // TODO: What does this item do?
            .symbol = '!', 
            .name = "potion of speed", 
            .color = GREEN_BLACK_PAIR,
            .itemClass = ItemClass::POTION,
            .value = 200,
            .pickable_type = PickableType::CONSUMABLE,
            .consumable_effect = ConsumableEffect::NONE}},

        // === SCROLLS ===
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
            .scroll_animation = ScrollAnimation::LIGHTNING}},

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
            .scroll_animation = ScrollAnimation::EXPLOSION}},

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
            .scroll_animation = ScrollAnimation::NONE}},

        {ItemId::SCROLL_TELEPORT, {
            .symbol = '?', 
            .name = "scroll of teleportation", 
            .color = MAGENTA_BLACK_PAIR, 
            .itemClass = ItemClass::SCROLL, 
            .value = 200, 
            .pickable_type = PickableType::TELEPORTER}},

        // === WEAPONS - MELEE ===
        {ItemId::DAGGER, {
            .symbol = '/', 
            .name = "dagger",
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::DAGGER, 
            .value = 2, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::ONE_HANDED, 
            .weapon_size = WeaponSize::TINY}},

        {ItemId::SHORT_SWORD, {
            .symbol = '/', 
            .name = "short sword",
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::SWORD,
            .value = 10,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED, 
            .weapon_size = WeaponSize::SMALL}},

        {ItemId::LONG_SWORD, {
            .symbol = '/', 
            .name = "long sword",
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::SWORD, 
            .value = 15, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::ONE_HANDED, 
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::BASTARD_SWORD, {
            .symbol = '/', 
            .name = "bastard sword", 
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::SWORD, 
            .value = 25, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::ONE_HANDED, 
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::SCIMITAR, {
            .symbol = '/',
            .name = "scimitar",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::SWORD,
            .value = 15,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::RAPIER, {
            .symbol = '/',
            .name = "rapier",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::SWORD,
            .value = 15,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::GREAT_SWORD, {
            .symbol = '/',
            .name = "greatsword",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::GREAT_SWORD, 
            .value = 50,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::TWO_HANDED,
            .weapon_size = WeaponSize::LARGE}},

        {ItemId::TWO_HANDED_SWORD, {
            .symbol = '/', 
            .name = "two-handed sword", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::GREAT_SWORD, 
            .value = 45, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::TWO_HANDED, 
            .weapon_size = WeaponSize::LARGE}},

        {ItemId::HAND_AXE, {
            .symbol = '/', 
            .name = "hand axe", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::AXE, 
            .value = 8, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::ONE_HANDED, 
            .weapon_size = WeaponSize::SMALL}},

        {ItemId::BATTLE_AXE, {
            .symbol = '/',
            .name = "battle axe",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::AXE,
            .value = 25,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::GREAT_AXE, {
            .symbol = '/',
            .name = "great axe",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::AXE, 
            .value = 40, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::TWO_HANDED, 
            .weapon_size = WeaponSize::LARGE}},

        {ItemId::CLUB, {
            .symbol = '|', 
            .name = "club", 
            .color = BROWN_BLACK_PAIR, 
            .itemClass = ItemClass::MACE, 
            .value = 1, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::ONE_HANDED, 
            .weapon_size = WeaponSize::SMALL}},

        {ItemId::MACE, {
            .symbol = '|',
            .name = "mace",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::MACE,
            .value = 8,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::WAR_HAMMER, {
            .symbol = '|',
            .name = "war hammer",
            .color = WHITE_BLACK_PAIR,
            .itemClass = ItemClass::HAMMER,
            .value = 20,
            .pickable_type = PickableType::WEAPON,
            .hand_requirement = HandRequirement::ONE_HANDED,
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::MORNING_STAR, {
            .symbol = '|', 
            .name = "morning star", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::MACE, 
            .value = 10, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::ONE_HANDED, 
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::FLAIL, {
            .symbol = '|', 
            .name = "flail", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::MACE, 
            .value = 8, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::ONE_HANDED, 
            .weapon_size = WeaponSize::MEDIUM}},

        {ItemId::STAFF, {
            .symbol = '/', 
            .name = "staff", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::STAFF, 
            .value = 6, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::TWO_HANDED, 
            .weapon_size = WeaponSize::LARGE}},

        {ItemId::QUARTERSTAFF, {
            .symbol = '/', 
            .name = "quarterstaff",
            .color = BROWN_BLACK_PAIR, 
            .itemClass = ItemClass::STAFF, 
            .value = 1, 
            .pickable_type = PickableType::WEAPON, 
            .hand_requirement = HandRequirement::TWO_HANDED, 
            .weapon_size = WeaponSize::LARGE}},

        // === WEAPONS - RANGED ===
        {ItemId::SLING, { // TODO: check validity of item.
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
            .weapon_size = WeaponSize::LARGE}},

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


        // === SHIELDS ===
        {ItemId::MEDIUM_SHIELD, {
            .symbol = '[', 
            .name = "shield", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::SHIELD, 
            .value = 10, 
            .pickable_type = PickableType::SHIELD}},


        // === ARMOR ===
        {ItemId::PADDED_ARMOR, {
            .symbol = '[', 
            .name = "padded armor", 
            .color = BROWN_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 4, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::LEATHER_ARMOR, {
            .symbol = '[',
            .name = "leather armor",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 5,
            .pickable_type = PickableType::ARMOR}},

        {ItemId::STUDDED_LEATHER, {
            .symbol = '[',
            .name = "studded leather",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 20,
            .pickable_type = PickableType::ARMOR}},

        {ItemId::HIDE_ARMOR, {
            .symbol = '[',
            .name = "hide armor",
            .color = BROWN_BLACK_PAIR,
            .itemClass = ItemClass::ARMOR,
            .value = 15, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::RING_MAIL, {
            .symbol = '[', 
            .name = "ring mail", 
            .color = BROWN_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 30, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::SCALE_MAIL, {
            .symbol = '[', 
            .name = "scale mail", 
            .color = BROWN_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 45, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::CHAIN_MAIL, {
            .symbol = '[', 
            .name = "chain mail", 
            .color = BROWN_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 75, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::BRIGANDINE, {
            .symbol = '[', 
            .name = "brigandine", 
            .color = BROWN_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 120, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::SPLINT_MAIL, {
            .symbol = '[', 
            .name = "splint mail", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 80, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::BANDED_MAIL, {
            .symbol = '[', 
            .name = "banded mail", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 90, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::PLATE_MAIL, {
            .symbol = '[', 
            .name = "plate mail", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 400, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::FIELD_PLATE, {
            .symbol = '[', 
            .name = "field plate", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 2000, 
            .pickable_type = PickableType::ARMOR}},

        {ItemId::FULL_PLATE, {
            .symbol = '[', 
            .name = "full plate", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::ARMOR, 
            .value = 4000, 
            .pickable_type = PickableType::ARMOR}},

        // === MAGICAL HELMS ===
        {ItemId::HELM_OF_BRILLIANCE, {
            .symbol = '^', 
            .name = "helm of brilliance", 
            .color = YELLOW_BLACK_PAIR, 
            .itemClass = ItemClass::HELMET, 
            .value = 12000, 
            .pickable_type = PickableType::MAGICAL_HELM, 
            .effect = MagicalEffect::BRILLIANCE}},

        {ItemId::HELM_OF_TELEPORTATION, {
            .symbol = '^', 
            .name = "helm of teleportation", 
            .color = MAGENTA_BLACK_PAIR, 
            .itemClass = ItemClass::HELMET, 
            .value = 7500, 
            .pickable_type = PickableType::MAGICAL_HELM, 
            .effect = MagicalEffect::TELEPORTATION}},

        {ItemId::HELM_OF_TELEPATHY, {
            .symbol = '^', 
            .name = "helm of telepathy",
            .color = BLUE_BLACK_PAIR, 
            .itemClass = ItemClass::HELMET, 
            .value = 8000, 
            .pickable_type = PickableType::MAGICAL_HELM, 
            .effect = MagicalEffect::TELEPATHY}},

        {ItemId::HELM_OF_UNDERWATER_ACTION, {
            .symbol = '^', 
            .name = "helm of underwater action", 
            .color = CYAN_BLACK_PAIR, 
            .itemClass = ItemClass::HELMET, 
            .value = 6000, 
            .pickable_type = PickableType::MAGICAL_HELM, 
            .effect = MagicalEffect::UNDERWATER_ACTION}},

        // === MAGICAL RINGS ===
        {ItemId::RING_OF_PROTECTION_PLUS_1, {
            .symbol = '=', 
            .name = "ring of protection +1", 
            .color = CYAN_BLACK_PAIR, 
            .itemClass = ItemClass::RING, 
            .value = 2000, 
            .pickable_type = PickableType::MAGICAL_RING, 
            .effect = MagicalEffect::PROTECTION, 
            .effect_bonus = 1}},

        {ItemId::RING_OF_PROTECTION_PLUS_2, {
            .symbol = '=', 
            .name = "ring of protection +2", 
            .color = CYAN_BLACK_PAIR, 
            .itemClass = ItemClass::RING, 
            .value = 5000, 
            .pickable_type = PickableType::MAGICAL_RING, 
            .effect = MagicalEffect::PROTECTION, 
            .effect_bonus = 2}},

        {ItemId::RING_OF_FREE_ACTION, {
            .symbol = '=', 
            .name = "ring of free action", 
            .color = GREEN_BLACK_PAIR, 
            .itemClass = ItemClass::RING, 
            .value = 4000, 
            .pickable_type = PickableType::MAGICAL_RING, 
            .effect = MagicalEffect::FREE_ACTION}},

        {ItemId::RING_OF_REGENERATION, {
            .symbol = '=', 
            .name = "ring of regeneration", 
            .color = RED_BLACK_PAIR, 
            .itemClass = ItemClass::RING, 
            .value = 8000, 
            .pickable_type = PickableType::MAGICAL_RING, 
            .effect = MagicalEffect::REGENERATION}},

        {ItemId::RING_OF_INVISIBILITY, {
            .symbol = '=', 
            .name = "ring of invisibility", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::RING, 
            .value = 6000, 
            .pickable_type = PickableType::MAGICAL_RING, 
            .effect = MagicalEffect::INVISIBILITY}},

        {ItemId::RING_OF_FIRE_RESISTANCE, {
            .symbol = '=', 
            .name = "ring of fire resistance", 
            .color = RED_BLACK_PAIR, 
            .itemClass = ItemClass::RING, 
            .value = 5000, 
            .pickable_type = PickableType::MAGICAL_RING, 
            .effect = MagicalEffect::FIRE_RESISTANCE}},

        {ItemId::RING_OF_COLD_RESISTANCE, {
            .symbol = '=', 
            .name = "ring of cold resistance", 
            .color = CYAN_BLACK_PAIR, 
            .itemClass = ItemClass::RING, 
            .value = 5000, 
            .pickable_type = PickableType::MAGICAL_RING, 
            .effect = MagicalEffect::COLD_RESISTANCE}},

        {ItemId::RING_OF_SPELL_STORING, {
            .symbol = '=', 
            .name = "ring of spell storing", 
            .color = MAGENTA_BLACK_PAIR, 
            .itemClass = ItemClass::RING, 
            .value = 10000, 
            .pickable_type = PickableType::MAGICAL_RING, 
            .effect = MagicalEffect::SPELL_STORING}},

        // === AMULETS ===
        {ItemId::AMULET_OF_HEALTH, {
            .symbol = '"', 
            .name = "amulet of health", 
            .color = RED_BLACK_PAIR, 
            .itemClass = ItemClass::AMULET, 
            .value = 200, 
            .pickable_type = PickableType::JEWELRY_AMULET, 
            .con_bonus = 1}},

        {ItemId::AMULET_OF_WISDOM, {
            .symbol = '"', 
            .name = "amulet of wisdom", 
            .color = BLUE_BLACK_PAIR, 
            .itemClass = ItemClass::AMULET, 
            .value = 200, 
            .pickable_type = PickableType::JEWELRY_AMULET, 
            .wis_bonus = 1}},

        {ItemId::AMULET_OF_PROTECTION, {
            .symbol = '"', 
            .name = "amulet of protection", 
            .color = CYAN_BLACK_PAIR, 
            .itemClass = ItemClass::AMULET, 
            .value = 150, 
            .pickable_type = PickableType::JEWELRY_AMULET}},

        {ItemId::AMULET_OF_OGRE_POWER, {
            .symbol = '"', 
            .name = "amulet of ogre power", 
            .color = RED_BLACK_PAIR, 
            .itemClass = ItemClass::AMULET, 
            .value = 3000, 
            .pickable_type = PickableType::JEWELRY_AMULET, 
            .effect = MagicalEffect::PROTECTION, 
            .effect_bonus = 1, 
            .str_bonus = 18, 
            .is_set_mode = true}},

        {ItemId::AMULET_OF_YENDOR, {
            .symbol = '"', 
            .name = "Amulet of Yendor", 
            .color = YELLOW_BLACK_PAIR, 
            .itemClass = ItemClass::QUEST_ITEM, 
            .value = 50000, 
            .pickable_type = PickableType::QUEST_ITEM}},

        // === GAUNTLETS ===
        {ItemId::GAUNTLETS_OF_OGRE_POWER, {
            .symbol = '[', 
            .name = "gauntlets of ogre power", 
            .color = RED_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, 
            .value = 3000, 
            .pickable_type = PickableType::GAUNTLETS, 
            .str_bonus = 18, 
            .is_set_mode = true}},

        {ItemId::GAUNTLETS_OF_DEXTERITY, {
            .symbol = '[', 
            .name = "gauntlets of dexterity", 
            .color = GREEN_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, 
            .value = 2000, 
            .pickable_type = PickableType::GAUNTLETS, 
            .dex_bonus = 2}},

        {ItemId::GAUNTLETS_OF_SWIMMING_AND_CLIMBING, {
            .symbol = '[', 
            .name = "gauntlets of swimming and climbing", 
            .color = BLUE_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, 
            .value = 3000, 
            .pickable_type = PickableType::GAUNTLETS, 
            .str_bonus = 2}},

        {ItemId::GAUNTLETS_OF_FUMBLING, {
            .symbol = '[', 
            .name = "gauntlets of fumbling", 
            .color = RED_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, 
            .value = 1, 
            .pickable_type = PickableType::GAUNTLETS, 
            .dex_bonus = -2}},

        // === GIRDLES ===
        {ItemId::GIRDLE_OF_HILL_GIANT_STRENGTH, {
            .symbol = '[', 
            .name = "girdle of hill giant strength", 
            .color = BROWN_BLACK_PAIR, 
            .itemClass = ItemClass::GIRDLE, 
            .value = 5000, 
            .pickable_type = PickableType::GIRDLE, 
            .str_bonus = 19, 
            .is_set_mode = true}},

        {ItemId::GIRDLE_OF_STONE_GIANT_STRENGTH, {
            .symbol = '[', 
            .name = "girdle of stone giant strength", 
            .color = BROWN_BLACK_PAIR, 
            .itemClass = ItemClass::GIRDLE, 
            .value = 6000, 
            .pickable_type = PickableType::GIRDLE, 
            .str_bonus = 20, 
            .is_set_mode = true}},

        {ItemId::GIRDLE_OF_FROST_GIANT_STRENGTH, {
            .symbol = '[', 
            .name = "girdle of frost giant strength", 
            .color = CYAN_BLACK_PAIR, 
            .itemClass = ItemClass::GIRDLE, 
            .value = 8000, 
            .pickable_type = PickableType::GIRDLE, 
            .str_bonus = 21, 
            .is_set_mode = true}},

        {ItemId::GIRDLE_OF_FIRE_GIANT_STRENGTH, {
            .symbol = '[', 
            .name = "girdle of fire giant strength", 
            .color = RED_BLACK_PAIR, 
            .itemClass = ItemClass::GIRDLE, 
            .value = 10000, 
            .pickable_type = PickableType::GIRDLE, 
            .str_bonus = 22, 
            .is_set_mode = true}},

        {ItemId::GIRDLE_OF_CLOUD_GIANT_STRENGTH, {
            .symbol = '[', 
            .name = "girdle of cloud giant strength", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::GIRDLE, 
            .value = 12000, 
            .pickable_type = PickableType::GIRDLE, 
            .str_bonus = 23, 
            .is_set_mode = true}},

        {ItemId::GIRDLE_OF_STORM_GIANT_STRENGTH, {
            .symbol = '[', 
            .name = "girdle of storm giant strength", 
            .color = YELLOW_BLACK_PAIR, 
            .itemClass = ItemClass::GIRDLE, 
            .value = 15000, 
            .pickable_type = PickableType::GIRDLE, 
            .str_bonus = 24, 
            .is_set_mode = true}},

        // === BOOTS ===
        {ItemId::BOOTS_OF_SPEED, {
            .symbol = '[', 
            .name = "boots of speed", 
            .color = GREEN_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, // TODO: why gauntlets on boots?
            .value = 5000, 
            .pickable_type = PickableType::GAUNTLETS, 
            .dex_bonus = 2}},

        {ItemId::BOOTS_OF_ELVENKIND, {
            .symbol = '[', 
            .name = "boots of elvenkind", 
            .color = GREEN_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, 
            .value = 3000, 
            .pickable_type = PickableType::GAUNTLETS, 
            .dex_bonus = 1}},

        {ItemId::BOOTS_OF_LEVITATION, {
            .symbol = '[', 
            .name = "boots of levitation", 
            .color = WHITE_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, 
            .value = 4000, 
            .pickable_type = PickableType::GAUNTLETS}},

        // === CLOAKS ===
        {ItemId::CLOAK_OF_PROTECTION, {
            .symbol = '[', 
            .name = "cloak of protection", 
            .color = CYAN_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, 
            .value = 3000, 
            .pickable_type = PickableType::GAUNTLETS, 
            .effect = MagicalEffect::PROTECTION, 
            .effect_bonus = 1}},

        {ItemId::CLOAK_OF_DISPLACEMENT, {
            .symbol = '[', 
            .name = "cloak of displacement", 
            .color = MAGENTA_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, 
            .value = 5000, 
            .pickable_type = PickableType::GAUNTLETS}},

        {ItemId::CLOAK_OF_ELVENKIND, {
            .symbol = '[', 
            .name = "cloak of elvenkind", 
            .color = GREEN_BLACK_PAIR, 
            .itemClass = ItemClass::GAUNTLETS, 
            .value = 4000, 
            .pickable_type = PickableType::GAUNTLETS, 
            .dex_bonus = 1}},

        // === FOOD ===
        {ItemId::FOOD_RATION, {
            .symbol = '%', 
            .name = "ration", 
            .color = WHITE_GREEN_PAIR, 
            .itemClass = ItemClass::FOOD, 
            .value = 10, 
            .pickable_type = PickableType::FOOD, 
            .nutrition_value = 300}},

        {ItemId::FRUIT, {
            .symbol = '%', 
            .name = "fruit", 
            .color = GREEN_BLACK_PAIR, 
            .itemClass = ItemClass::FOOD, 
            .value = 3, 
            .pickable_type = PickableType::FOOD, 
            .nutrition_value = 100}},

        {ItemId::BREAD, {
            .symbol = '%', 
            .name = "bread", 
            .color = RED_YELLOW_PAIR, 
            .itemClass = ItemClass::FOOD, 
            .value = 5, 
            .pickable_type = PickableType::FOOD, 
            .nutrition_value = 200}},

        {ItemId::MEAT, {
            .symbol = '%', 
            .name = "meat", 
            .color = RED_BLACK_PAIR, 
            .itemClass = ItemClass::FOOD, 
            .value = 8, 
            .pickable_type = PickableType::FOOD, 
            .nutrition_value = 250}},

        // === TREASURE ===
        {ItemId::GOLD, {
            .symbol = '$', 
            .name = "gold pile", 
            .color = YELLOW_BLACK_PAIR, 
            .itemClass = ItemClass::GOLD, 
            .value = 0, 
            .pickable_type = PickableType::GOLD}},

    };

    template<typename T>
    std::unique_ptr<T> create_stat_item(const ItemParams& p)
    {
        auto item = std::make_unique<T>();
        item->str_bonus = p.str_bonus;
        item->dex_bonus = p.dex_bonus;
        item->con_bonus = p.con_bonus;
        item->int_bonus = p.int_bonus;
        item->wis_bonus = p.wis_bonus;
        item->cha_bonus = p.cha_bonus;
        item->effect = p.effect;
        item->bonus = p.effect_bonus;
        item->is_set_mode = p.is_set_mode;
        return item;
    }

    std::unique_ptr<Pickable> create_pickable_from_blueprint(ItemId itemId, const ItemParams& params)
    {
        switch (params.pickable_type)
        {
            case PickableType::CONSUMABLE:
                return std::make_unique<Consumable>(
                    params.consumable_effect,
                    params.heal_amount,
                    params.duration,
                    params.consumable_buff_type,
                    params.is_set_mode
                );

            case PickableType::TARGETED_SCROLL:
                return std::make_unique<TargetedScroll>(
                    params.target_mode,
                    params.scroll_animation,
                    params.range,
                    params.damage,
                    params.confuse_turns
                );

            case PickableType::TELEPORTER:
                return std::make_unique<Teleporter>();

            case PickableType::WEAPON:
                return std::make_unique<Weapon>(params.ranged, params.hand_requirement, params.weapon_size);

            case PickableType::SHIELD:
                return std::make_unique<Shield>();

            case PickableType::ARMOR:
                // Armor type determined by item's AC value via load() - use LeatherArmor as base
                // load() from JSON will restore actual AC; ItemId stored on Item drives display
                return std::make_unique<LeatherArmor>();

            case PickableType::QUEST_ITEM:
                return std::make_unique<Amulet>();

            case PickableType::MAGICAL_HELM:
                return std::make_unique<MagicalHelm>(params.effect, params.effect_bonus);

            case PickableType::MAGICAL_RING:
                return std::make_unique<MagicalRing>(params.effect, params.effect_bonus);

            case PickableType::JEWELRY_AMULET:
                return create_stat_item<JewelryAmulet>(params);

            case PickableType::GAUNTLETS:
                return create_stat_item<Gauntlets>(params);

            case PickableType::GIRDLE:
                return create_stat_item<Girdle>(params);

            case PickableType::FOOD:
                return std::make_unique<Food>(params.nutrition_value);

            case PickableType::GOLD:
                return nullptr;

            default:
                throw std::runtime_error("Unknown pickable type: " + std::to_string(static_cast<int>(params.pickable_type)));
        }
    }

    std::unique_ptr<Item> create_from_blueprint(Vector2D pos, ItemId itemId)
    {
        const auto& params = ItemRegistry.at(itemId);
        auto item = std::make_unique<Item>(pos, ActorData{params.symbol, std::string{params.name}, params.color});
        item->pickable = create_pickable_from_blueprint(itemId, params);
        item->itemId = itemId;
        item->itemClass = params.itemClass;
        item->set_value(params.value);
        item->base_value = params.value;
        return item;
    }
}

const ItemParams& ItemCreator::get_params(ItemId itemId)
{
    return ItemRegistry.at(itemId);
}

std::unique_ptr<Item> ItemCreator::create(ItemId itemId, Vector2D pos)
{
    // Validate ItemId exists in registry
    if (ItemRegistry.find(itemId) == ItemRegistry.end())
    {
        throw std::invalid_argument("ItemId not found in registry: " + std::to_string(static_cast<int>(itemId)));
    }
    return create_from_blueprint(pos, itemId);
}

std::unique_ptr<Item> ItemCreator::create_with_gold_amount(Vector2D pos, int goldAmount)
{
    const auto& params = ItemRegistry.at(ItemId::GOLD);
    auto item = std::make_unique<Item>(pos, ActorData{params.symbol, std::string{params.name}, params.color});
    item->pickable = std::make_unique<Gold>(goldAmount);
    item->itemId = ItemId::GOLD;
    item->itemClass = params.itemClass;
    item->set_value(goldAmount);
    item->base_value = goldAmount;
    return item;
}

std::unique_ptr<Item> ItemCreator::create_enhanced_weapon(ItemId weaponId, Vector2D pos, int enhancementLevel)
{
    auto weapon = create(weaponId, pos);
    weapon->set_value(calculate_enhanced_value(weapon->base_value, enhancementLevel));
    weapon->actorData.name = "+" + std::to_string(enhancementLevel) + " " + weapon->actorData.name;
    weapon->actorData.color = WHITE_GREEN_PAIR;  // Enhanced items have green color
    return weapon;
}

std::unique_ptr<Item> ItemCreator::create_with_enhancement(ItemId itemId, Vector2D pos, PrefixType prefix, SuffixType suffix)
{
    auto item = create(itemId, pos);
    ItemEnhancement enhancement(prefix, suffix);
    item->apply_enhancement(enhancement);
    return item;
}

std::unique_ptr<Item> ItemCreator::create_gold_pile(Vector2D pos, GameContext& ctx)
{
    const int goldAmount = ctx.dice->roll(5, 20);
    return create_with_gold_amount(pos, goldAmount);
}

std::unique_ptr<Item> ItemCreator::create_random_weapon(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    static const std::vector<ItemId> weapons = {
        ItemId::DAGGER, ItemId::SHORT_SWORD, ItemId::LONG_SWORD, ItemId::GREAT_SWORD,
        ItemId::BATTLE_AXE, ItemId::GREAT_AXE, ItemId::WAR_HAMMER, ItemId::MACE, ItemId::STAFF, ItemId::LONG_BOW
    };
    const ItemId weaponId = weapons[ctx.dice->roll(0, static_cast<int>(weapons.size()) - 1)];
    return create(weaponId, pos);
}

std::unique_ptr<Item> ItemCreator::create_random_armor(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    // Only include armor types that are actually implemented in the factory
    static const std::vector<ItemId> armor = {
        ItemId::LEATHER_ARMOR,
        ItemId::CHAIN_MAIL,
        ItemId::PLATE_MAIL
    };
    const ItemId armorId = armor[ctx.dice->roll(0, static_cast<int>(armor.size()) - 1)];
    return create(armorId, pos);
}

// TODO: same as scrolls...
std::unique_ptr<Item> ItemCreator::create_random_potion(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    static const std::vector<ItemId> potions = {
        ItemId::HEALTH_POTION, ItemId::MANA_POTION, ItemId::INVISIBILITY_POTION
    };
    const ItemId potionId = potions[ctx.dice->roll(0, static_cast<int>(potions.size()) - 1)];
    return create(potionId, pos);
}

// TODO: can't we query? why always add new scrolls?
std::unique_ptr<Item> ItemCreator::create_random_scroll(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    static const std::vector<ItemId> scrolls = {
        ItemId::SCROLL_LIGHTNING, ItemId::SCROLL_FIREBALL, ItemId::SCROLL_CONFUSION, ItemId::SCROLL_TELEPORT
    };
    const ItemId scrollId = scrolls[ctx.dice->roll(0, static_cast<int>(scrolls.size()) - 1)];
    return create(scrollId, pos);
}

std::unique_ptr<Item> ItemCreator::create_weapon_with_enhancement_chance(Vector2D pos, GameContext& ctx, int dungeonLevel)
{
    auto weapon = create_random_weapon(pos, ctx, dungeonLevel);
    const int enhancementChance = calculate_enhancement_chance(dungeonLevel);
    if (ctx.dice->roll(1, 100) <= enhancementChance)
    {
        const int enhancementLevel = determine_enhancement_level(ctx, dungeonLevel);
        weapon->set_value(calculate_enhanced_value(weapon->base_value, enhancementLevel));
        weapon->actorData.name = "+" + std::to_string(enhancementLevel) + " " + weapon->actorData.name;
    }
    return weapon;
}

int ItemCreator::calculate_enhancement_chance(int dungeonLevel)
{
    return std::min(2 + (dungeonLevel * 3), 35);
}

int ItemCreator::determine_enhancement_level(GameContext& ctx, int dungeonLevel)
{
    const int roll = ctx.dice->roll(1, 100);
    if (dungeonLevel >= 10 && roll <= 5) return 3;
    if (dungeonLevel >= 5 && roll <= 20) return 2;
    return 1;
}

int ItemCreator::calculate_enhanced_value(int baseValue, int enhancementLevel)
{
    return baseValue * (1 + enhancementLevel);
}
