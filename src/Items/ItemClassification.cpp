// ItemClassification.cpp
#include <string>
#include <unordered_map>

#include "ItemClassification.h"
#include "Weapons.h"

namespace ItemClassificationUtils
{

ItemCategory get_category(ItemClass itemClass)
{
	switch (itemClass)
	{
	case ItemClass::DAGGER:
	case ItemClass::SWORD:
	case ItemClass::GREAT_SWORD:
	case ItemClass::AXE:
	case ItemClass::HAMMER:
	case ItemClass::MACE:
	case ItemClass::STAFF:
	case ItemClass::BOW:
	case ItemClass::CROSSBOW:
		return ItemCategory::WEAPON;

	case ItemClass::ARMOR:
		return ItemCategory::ARMOR;

	case ItemClass::HELMET:
		return ItemCategory::HELMET;

	case ItemClass::SHIELD:
		return ItemCategory::SHIELD;

	case ItemClass::GAUNTLETS:
		return ItemCategory::GAUNTLETS;

	case ItemClass::GIRDLE:
		return ItemCategory::GIRDLE;

	case ItemClass::POTION:
	case ItemClass::FOOD:
		return ItemCategory::CONSUMABLE;

	case ItemClass::SCROLL:
		return ItemCategory::SCROLL;

	case ItemClass::AMULET:
	case ItemClass::RING:
		return ItemCategory::JEWELRY;

	case ItemClass::GOLD_COIN:
	case ItemClass::GEM:
		return ItemCategory::TREASURE;

	case ItemClass::TOOL:
		return ItemCategory::TOOL;

	case ItemClass::QUEST_ITEM:
		return ItemCategory::QUEST_ITEM;

	default:
		return ItemCategory::UNKNOWN;
	}
}

bool can_equip_to_right_hand(ItemClass itemClass)
{
	return is_weapon(itemClass) || is_shield(itemClass);
}

bool can_equip_to_left_hand(ItemClass itemClass)
{
	if (is_shield(itemClass))
		return true;
	if (!is_weapon(itemClass))
		return false;

	return !is_two_handed_weapon(itemClass);
}

bool can_equip_to_body(ItemClass itemClass)
{
	return is_armor(itemClass);
}

bool is_two_handed_weapon(ItemClass itemClass)
{
	switch (itemClass)
	{
	case ItemClass::GREAT_SWORD:
	case ItemClass::AXE:
	case ItemClass::BOW:
	case ItemClass::CROSSBOW:
	case ItemClass::STAFF:
		return true;
	default:
		return false;
	}
}

ItemClass item_class_from_string(const std::string& typeName)
{
	static const std::unordered_map<std::string, ItemClass> classMap = {
		{ "dagger", ItemClass::DAGGER },
		{ "sword", ItemClass::SWORD },
		{ "great_sword", ItemClass::GREAT_SWORD },
		{ "axe", ItemClass::AXE },
		{ "hammer", ItemClass::HAMMER },
		{ "mace", ItemClass::MACE },
		{ "staff", ItemClass::STAFF },
		{ "bow", ItemClass::BOW },
		{ "crossbow", ItemClass::CROSSBOW },
		{ "armor", ItemClass::ARMOR },
		{ "shield", ItemClass::SHIELD },
		{ "helmet", ItemClass::HELMET },
		{ "ring", ItemClass::RING },
		{ "amulet", ItemClass::AMULET },
		{ "gauntlets", ItemClass::GAUNTLETS },
		{ "girdle", ItemClass::GIRDLE },
		{ "potion", ItemClass::POTION },
		{ "scroll", ItemClass::SCROLL },
		{ "food", ItemClass::FOOD },
		{ "gold", ItemClass::GOLD_COIN },
		{ "gem", ItemClass::GEM },
		{ "tool", ItemClass::TOOL },
		{ "quest_item", ItemClass::QUEST_ITEM }
	};

	return classMap.contains(typeName) ? classMap.at(typeName) : ItemClass::UNKNOWN;
}

} // namespace ItemClassificationUtils
