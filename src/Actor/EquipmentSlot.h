#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

// file: EquipmentSlot.h
//
// The places an item can sit on a creature, and the codec that moves a slot
// between C++ and JSON.
//
// Usage -- reading a slot authored in a data file:
//
//   EquipmentSlot slot = parse_equipment_slot("right_hand"); // throws if unknown
//   encode_equipment_slot(slot);                             // -> "right_hand"

enum class EquipmentSlot
{
	HEAD,           // Helmets, hats
	NECK,           // Amulets, necklaces
	BODY,           // Armor (chain mail, plate mail, etc.)
	GIRDLE,         // Belts
	CLOAK,          // Cloaks, robes
	RIGHT_HAND,     // Main weapon
	LEFT_HAND,      // Shield or off-hand weapon
	RIGHT_RING,     // Rings
	LEFT_RING,      // Rings
	BRACERS,        // Bracers
	GAUNTLETS,      // Gloves, gauntlets
	BOOTS,          // Boots, shoes
	MISSILE_WEAPON, // Bows, crossbows
	MISSILES,       // Arrows, bolts
	TOOL,           // Tools, instruments
	NONE
};

// The JSON spelling of a slot. snake_case, like every other data identifier.
inline std::string_view encode_equipment_slot(EquipmentSlot slot)
{
	switch (slot)
	{
	case EquipmentSlot::HEAD:
	{
		return "head";
	}
	case EquipmentSlot::NECK:
	{
		return "neck";
	}
	case EquipmentSlot::BODY:
	{
		return "body";
	}
	case EquipmentSlot::GIRDLE:
	{
		return "girdle";
	}
	case EquipmentSlot::CLOAK:
	{
		return "cloak";
	}
	case EquipmentSlot::RIGHT_HAND:
	{
		return "right_hand";
	}
	case EquipmentSlot::LEFT_HAND:
	{
		return "left_hand";
	}
	case EquipmentSlot::RIGHT_RING:
	{
		return "right_ring";
	}
	case EquipmentSlot::LEFT_RING:
	{
		return "left_ring";
	}
	case EquipmentSlot::BRACERS:
	{
		return "bracers";
	}
	case EquipmentSlot::GAUNTLETS:
	{
		return "gauntlets";
	}
	case EquipmentSlot::BOOTS:
	{
		return "boots";
	}
	case EquipmentSlot::MISSILE_WEAPON:
	{
		return "missile_weapon";
	}
	case EquipmentSlot::MISSILES:
	{
		return "missiles";
	}
	case EquipmentSlot::TOOL:
	{
		return "tool";
	}
	case EquipmentSlot::NONE:
	{
		return "none";
	}
	}

	throw std::runtime_error("unknown EquipmentSlot value");
}

// Reads a slot authored in a data file. Throws rather than defaulting, so a
// misspelled slot fails at load instead of silently going nowhere.
inline EquipmentSlot parse_equipment_slot(std::string_view name)
{
	for (int index = 0; index <= static_cast<int>(EquipmentSlot::NONE); ++index)
	{
		const EquipmentSlot slot = static_cast<EquipmentSlot>(index);
		if (encode_equipment_slot(slot) == name)
		{
			return slot;
		}
	}

	throw std::runtime_error(std::format("unknown equipment slot '{}'", name));
}
