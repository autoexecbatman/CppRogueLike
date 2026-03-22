#pragma once
#include <string>

#include "Weapons.h" // For WeaponSize enum

// CLASSIFICATION - Category/type of item (no specific variants)
enum class ItemClass
{
	UNKNOWN = 0,

	// Weapons
	DAGGER,
	SWORD,
	GREAT_SWORD,
	AXE,
	HAMMER,
	MACE,
	STAFF,
	BOW,
	CROSSBOW,

	// Armor & Protection
	ARMOR,
	SHIELD,
	HELMET,

	// Wearables
	RING,
	AMULET,
	GAUNTLETS,
	GIRDLE,

	// Consumables
	POTION,
	SCROLL,
	FOOD,

	// Other
	GOLD_COIN,
	GEM,
	TOOL,
	QUEST_ITEM,
};

// Item category for grouping
enum class ItemCategory
{
	UNKNOWN = 0,
	WEAPON,
	ARMOR,
	HELMET,
	SHIELD,
	GAUNTLETS,
	GIRDLE,
	CONSUMABLE,
	SCROLL,
	JEWELRY,
	TREASURE,
	TOOL,
	QUEST_ITEM
};

// Utility functions for item classification
namespace ItemClassificationUtils
{
// Get category from item class
ItemCategory get_category(ItemClass itemClass);

// Type checking functions - now simple and clean
inline bool is_weapon(ItemClass itemClass)
{
	return itemClass == ItemClass::DAGGER ||
		itemClass == ItemClass::SWORD ||
		itemClass == ItemClass::GREAT_SWORD ||
		itemClass == ItemClass::AXE ||
		itemClass == ItemClass::HAMMER ||
		itemClass == ItemClass::MACE ||
		itemClass == ItemClass::STAFF ||
		itemClass == ItemClass::BOW ||
		itemClass == ItemClass::CROSSBOW;
}

inline bool is_armor(ItemClass itemClass)
{
	return itemClass == ItemClass::ARMOR;
}
inline bool is_helmet(ItemClass itemClass)
{
	return itemClass == ItemClass::HELMET;
}
inline bool is_shield(ItemClass itemClass)
{
	return itemClass == ItemClass::SHIELD;
}
inline bool is_gauntlets(ItemClass itemClass)
{
	return itemClass == ItemClass::GAUNTLETS;
}
inline bool is_girdle(ItemClass itemClass)
{
	return itemClass == ItemClass::GIRDLE;
}
inline bool is_potion(ItemClass itemClass)
{
	return itemClass == ItemClass::POTION;
}
inline bool is_scroll(ItemClass itemClass)
{
	return itemClass == ItemClass::SCROLL;
}
inline bool is_food(ItemClass itemClass)
{
	return itemClass == ItemClass::FOOD;
}
inline bool is_amulet(ItemClass itemClass)
{
	return itemClass == ItemClass::AMULET;
}
inline bool is_ring(ItemClass itemClass)
{
	return itemClass == ItemClass::RING;
}
inline bool is_treasure(ItemClass itemClass)
{
	return itemClass == ItemClass::GOLD_COIN || itemClass == ItemClass::GEM;
}
inline bool is_tool(ItemClass itemClass)
{
	return itemClass == ItemClass::TOOL;
}
inline bool is_quest_item(ItemClass itemClass)
{
	return itemClass == ItemClass::QUEST_ITEM;
}

// Composite type checking
inline bool is_consumable(ItemClass itemClass)
{
	return is_potion(itemClass) || is_scroll(itemClass) || is_food(itemClass);
}
inline bool is_jewelry(ItemClass itemClass)
{
	return is_ring(itemClass) || is_amulet(itemClass);
}

// Ranged weapon checking
inline bool is_ranged_weapon(ItemClass itemClass)
{
	return itemClass == ItemClass::BOW || itemClass == ItemClass::CROSSBOW;
}

// Equipment slot detection
bool can_equip_to_right_hand(ItemClass itemClass);
bool can_equip_to_left_hand(ItemClass itemClass);
bool can_equip_to_body(ItemClass itemClass);
bool is_two_handed_weapon(ItemClass itemClass);

// Convert from string (for loading/creation)
ItemClass item_class_from_string(const std::string& typeName);
} // namespace ItemClassificationUtils
