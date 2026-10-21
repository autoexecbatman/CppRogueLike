#pragma once

#include <cassert>

#include "EquipmentSlot.h"

// Which way an attack is being made. The attacker's equipment cannot answer this:
// somebody carrying a bow still swings the sword in their hand when they walk into
// a goblin, and the same creature loosing an arrow at range is doing something
// else. Only the caller knows which one it started, so it says so.
enum class AttackKind
{
	MELEE, // Struck at touching range with whatever is in hand
	RANGED, // Loosed from the missile slot at a target further off
};

// The slot an attack of this kind is made from.
//
// Example:
//   weapon_slot_for(AttackKind::RANGED);   // -> EquipmentSlot::MISSILE_WEAPON
//   weapon_slot_for(AttackKind::MELEE);    // -> EquipmentSlot::RIGHT_HAND
[[nodiscard]] inline EquipmentSlot weapon_slot_for(AttackKind kind) noexcept
{
	switch (kind)
	{
	case AttackKind::RANGED:
	{
		return EquipmentSlot::MISSILE_WEAPON;
	}
	case AttackKind::MELEE:
	{
		return EquipmentSlot::RIGHT_HAND;
	}
	}

	assert(false && "weapon_slot_for: an AttackKind with no slot");
	return EquipmentSlot::RIGHT_HAND;
}
