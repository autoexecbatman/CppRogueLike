#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

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
	SLING,

	// Armor & Protection
	ARMOR,
	SHIELD,
	HELMET,

	// Wearables
	RING,
	AMULET,
	GAUNTLETS,
	BOOTS,
	CLOAK,
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

	// How many classes there are; names no class.
	COUNT,
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
	BOOTS,
	CLOAK,
	GIRDLE,
	CONSUMABLE,
	SCROLL,
	JEWELRY,
	TREASURE,
	TOOL,
	QUEST_ITEM
};

// One item class: its key in items.json and in a save, and the inventory
// heading it is listed under.
struct ItemClassRecord
{
	ItemClass itemClass{ ItemClass::UNKNOWN };
	std::string_view key{};
	ItemCategory category{ ItemCategory::UNKNOWN };
};

// How many classes there are, as a size.
inline constexpr std::size_t ITEM_CLASS_COUNT =
	static_cast<std::size_t>(ItemClass::COUNT);

// Every ItemClass, one row each in the order the enum declares them. This is
// the only list of the classes; everything else that names them reads it.
inline constexpr std::array<ItemClassRecord, ITEM_CLASS_COUNT> ITEM_CLASS_TABLE{
	{
		{ ItemClass::UNKNOWN, "unknown", ItemCategory::UNKNOWN },
		{ ItemClass::DAGGER, "dagger", ItemCategory::WEAPON },
		{ ItemClass::SWORD, "sword", ItemCategory::WEAPON },
		{ ItemClass::GREAT_SWORD, "great_sword", ItemCategory::WEAPON },
		{ ItemClass::AXE, "axe", ItemCategory::WEAPON },
		{ ItemClass::HAMMER, "hammer", ItemCategory::WEAPON },
		{ ItemClass::MACE, "mace", ItemCategory::WEAPON },
		{ ItemClass::STAFF, "staff", ItemCategory::WEAPON },
		{ ItemClass::BOW, "bow", ItemCategory::WEAPON },
		{ ItemClass::CROSSBOW, "crossbow", ItemCategory::WEAPON },
		{ ItemClass::SLING, "sling", ItemCategory::WEAPON },
		{ ItemClass::ARMOR, "armor", ItemCategory::ARMOR },
		{ ItemClass::SHIELD, "shield", ItemCategory::SHIELD },
		{ ItemClass::HELMET, "helmet", ItemCategory::HELMET },
		{ ItemClass::RING, "ring", ItemCategory::JEWELRY },
		{ ItemClass::AMULET, "amulet", ItemCategory::JEWELRY },
		{ ItemClass::GAUNTLETS, "gauntlets", ItemCategory::GAUNTLETS },
		{ ItemClass::BOOTS, "boots", ItemCategory::BOOTS },
		{ ItemClass::CLOAK, "cloak", ItemCategory::CLOAK },
		{ ItemClass::GIRDLE, "girdle", ItemCategory::GIRDLE },
		{ ItemClass::POTION, "potion", ItemCategory::CONSUMABLE },
		{ ItemClass::SCROLL, "scroll", ItemCategory::SCROLL },
		{ ItemClass::FOOD, "food", ItemCategory::CONSUMABLE },
		{ ItemClass::GOLD_COIN, "gold_coin", ItemCategory::TREASURE },
		{ ItemClass::GEM, "gem", ItemCategory::TREASURE },
		{ ItemClass::TOOL, "tool", ItemCategory::TOOL },
		{ ItemClass::QUEST_ITEM, "quest_item", ItemCategory::QUEST_ITEM },
	}
};

// Whether row n of the table holds class n. A class added to the enum without
// a row leaves a defaulted row, which fails this.
[[nodiscard]] constexpr bool is_item_class_table_in_enum_order()
{
	for (std::size_t row = 0; row < ITEM_CLASS_TABLE.size(); ++row)
	{
		if (ITEM_CLASS_TABLE[row].itemClass != static_cast<ItemClass>(row) ||
			ITEM_CLASS_TABLE[row].key.empty())
		{
			return false;
		}
	}
	return true;
}

static_assert(
	is_item_class_table_in_enum_order(),
	"ITEM_CLASS_TABLE needs one row per ItemClass, in enum order");

// The table's row for a class. COUNT has no row.
//
// Example:
//   item_class_record(ItemClass::BOOTS).key;   // -> "boots"
[[nodiscard]] constexpr const ItemClassRecord& item_class_record(
	ItemClass itemClass)
{
	return ITEM_CLASS_TABLE.at(static_cast<std::size_t>(itemClass));
}

// The classes alone, in table order.
[[nodiscard]] constexpr std::array<ItemClass, ITEM_CLASS_COUNT>
all_item_classes()
{
	std::array<ItemClass, ITEM_CLASS_COUNT> classes{};
	for (std::size_t row = 0; row < ITEM_CLASS_TABLE.size(); ++row)
	{
		classes[row] = ITEM_CLASS_TABLE[row].itemClass;
	}
	return classes;
}

// Every ItemClass, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them.
inline constexpr auto ALL_ITEM_CLASS = all_item_classes();

// The next ItemClass in that order, wrapping at the end.
//
// Example:
//   next_item_class(ItemClass::UNKNOWN);   // -> ItemClass::DAGGER
[[nodiscard]] inline ItemClass next_item_class(ItemClass value)
{
	const auto found = std::ranges::find(ALL_ITEM_CLASS, value);
	if (found == ALL_ITEM_CLASS.end() || found + 1 == ALL_ITEM_CLASS.end())
	{
		return ALL_ITEM_CLASS.front();
	}
	return *(found + 1);
}

// Utility functions for item classification
namespace ItemClassificationUtils
{
// Get category from item class
ItemCategory get_category(ItemClass itemClass);

// Type checking functions - now simple and clean
inline bool is_weapon(ItemClass itemClass)
{
	return itemClass == ItemClass::DAGGER || itemClass == ItemClass::SWORD ||
		itemClass == ItemClass::GREAT_SWORD || itemClass == ItemClass::AXE ||
		itemClass == ItemClass::HAMMER || itemClass == ItemClass::MACE ||
		itemClass == ItemClass::STAFF || itemClass == ItemClass::BOW ||
		itemClass == ItemClass::CROSSBOW || itemClass == ItemClass::SLING;
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

inline bool is_boots(ItemClass itemClass)
{
	return itemClass == ItemClass::BOOTS;
}

inline bool is_cloak(ItemClass itemClass)
{
	return itemClass == ItemClass::CLOAK;
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
	return itemClass == ItemClass::BOW || itemClass == ItemClass::CROSSBOW ||
		itemClass == ItemClass::SLING;
}

// Equipment slot detection
bool can_equip_to_right_hand(ItemClass itemClass);
bool can_equip_to_left_hand(ItemClass itemClass);
bool can_equip_to_body(ItemClass itemClass);
bool is_two_handed_weapon(ItemClass itemClass);
} // namespace ItemClassificationUtils

// The key a class is written under, in items.json and in a save.
//
// Example:
//   encode_item_class(ItemClass::GOLD_COIN);   // -> "gold_coin"
inline std::string_view encode_item_class(ItemClass itemClass)
{
	return item_class_record(itemClass).key;
}

// The class a key names. Throws std::runtime_error for a key no class has.
//
// Example:
//   parse_item_class("boots");     // -> ItemClass::BOOTS
//   parse_item_class("sandals");   // throws "unknown item_class 'sandals'"
inline ItemClass parse_item_class(std::string_view name)
{
	const auto found =
		std::ranges::find(ITEM_CLASS_TABLE, name, &ItemClassRecord::key);
	if (found == ITEM_CLASS_TABLE.end())
	{
		throw std::runtime_error(std::format("unknown item_class '{}'", name));
	}
	return found->itemClass;
}
