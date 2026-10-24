// ItemClassification.cpp
#include "ItemClassification.h"

namespace ItemClassificationUtils
{

ItemCategory get_category(ItemClass itemClass)
{
	return item_class_record(itemClass).category;
}

bool can_equip_to_right_hand(ItemClass itemClass)
{
	return is_weapon(itemClass) || is_shield(itemClass);
}

bool can_equip_to_left_hand(ItemClass itemClass)
{
	if (is_shield(itemClass))
	{
		return true;
	}
	if (!is_weapon(itemClass))
	{
		return false;
	}

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
	case ItemClass::BOW:
	case ItemClass::CROSSBOW:
	case ItemClass::STAFF:
	{
		return true;
	}
	default:
	{
		return false;
	}
	}
}

} // namespace ItemClassificationUtils
