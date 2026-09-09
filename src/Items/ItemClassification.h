#pragma once

#include <format>
#include <stdexcept>
#include <string_view>
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

inline std::string_view encode_item_class(ItemClass c)
{
	switch (c)
	{

	case ItemClass::UNKNOWN:
	{
		return "unknown";
	}

	case ItemClass::DAGGER:
	{
		return "dagger";
	}

	case ItemClass::SWORD:
	{
		return "sword";
	}

	case ItemClass::GREAT_SWORD:
	{
		return "great_sword";
	}

	case ItemClass::AXE:
	{
		return "axe";
	}

	case ItemClass::HAMMER:
	{
		return "hammer";
	}

	case ItemClass::MACE:
	{
		return "mace";
	}

	case ItemClass::STAFF:
	{
		return "staff";
	}

	case ItemClass::BOW:
	{
		return "bow";
	}

	case ItemClass::CROSSBOW:
	{
		return "crossbow";
	}

	case ItemClass::ARMOR:
	{
		return "armor";
	}

	case ItemClass::SHIELD:
	{
		return "shield";
	}

	case ItemClass::HELMET:
	{
		return "helmet";
	}

	case ItemClass::RING:
	{
		return "ring";
	}

	case ItemClass::AMULET:
	{
		return "amulet";
	}

	case ItemClass::GAUNTLETS:
	{
		return "gauntlets";
	}

	case ItemClass::GIRDLE:
	{
		return "girdle";
	}

	case ItemClass::POTION:
	{
		return "potion";
	}

	case ItemClass::SCROLL:
	{
		return "scroll";
	}

	case ItemClass::FOOD:
	{
		return "food";
	}

	case ItemClass::GOLD_COIN:
	{
		return "gold_coin";
	}

	case ItemClass::GEM:
	{
		return "gem";
	}

	case ItemClass::TOOL:
	{
		return "tool";
	}
	case ItemClass::QUEST_ITEM:
	{
		return "quest_item";
	}

	}

	return "unknown";
}

inline ItemClass parse_item_class(std::string_view s)
{
	if (s == "unknown")
	{
		return ItemClass::UNKNOWN;
	}
	if (s == "dagger")
	{
		return ItemClass::DAGGER;
	}
	if (s == "sword")
	{
		return ItemClass::SWORD;
	}
	if (s == "great_sword")
	{
		return ItemClass::GREAT_SWORD;
	}
	if (s == "axe")
	{
		return ItemClass::AXE;
	}
	if (s == "hammer")
	{
		return ItemClass::HAMMER;
	}
	if (s == "mace")
	{
		return ItemClass::MACE;
	}
	if (s == "staff")
	{
		return ItemClass::STAFF;
	}
	if (s == "bow")
	{
		return ItemClass::BOW;
	}
	if (s == "crossbow")
	{
		return ItemClass::CROSSBOW;
	}
	if (s == "armor")
	{
		return ItemClass::ARMOR;
	}
	if (s == "shield")
	{
		return ItemClass::SHIELD;
	}
	if (s == "helmet")
	{
		return ItemClass::HELMET;
	}
	if (s == "ring")
	{
		return ItemClass::RING;
	}
	if (s == "amulet")
	{
		return ItemClass::AMULET;
	}
	if (s == "gauntlets")
	{
		return ItemClass::GAUNTLETS;
	}
	if (s == "girdle")
	{
		return ItemClass::GIRDLE;
	}
	if (s == "potion")
	{
		return ItemClass::POTION;
	}
	if (s == "scroll")
	{
		return ItemClass::SCROLL;
	}
	if (s == "food")
	{
		return ItemClass::FOOD;
	}
	if (s == "gold_coin")
	{
		return ItemClass::GOLD_COIN;
	}
	if (s == "gem")
	{
		return ItemClass::GEM;
	}
	if (s == "tool")
	{
		return ItemClass::TOOL;
	}
	if (s == "quest_item")
	{
		return ItemClass::QUEST_ITEM;
	}

	throw std::runtime_error(std::format("unknown item_class '{}'", s));
}
