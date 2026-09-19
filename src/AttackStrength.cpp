// file: AttackStrength.cpp
#include "AttackStrength.h"

#include <algorithm>

#include "Item.h"
#include "ItemClassification.h"

AttackStrength::Adjustment AttackStrength::adjustment(const StrengthAttributes& row, AttackKind kind, const Item* missileWeapon)
{
	// A swing, and a missile thrown by hand or by nature, carries the arm's whole strength.
	const Adjustment wholeRow{ row.hitProb, row.dmgAdj };
	if (kind == AttackKind::MELEE || missileWeapon == nullptr)
	{
		return wholeRow;
	}

	switch (missileWeapon->itemClass)
	{
	// A weak arm cannot draw a bow fully; a strong one gains only from a bow made for it.
	case ItemClass::BOW:
	{
		return Adjustment{ std::min(row.hitProb, 0), std::min(row.dmgAdj, 0) };
	}

	// A crossbow is drawn by a machine, and a sling is not in the rule's list.
	case ItemClass::CROSSBOW:
	case ItemClass::SLING:
	{
		return Adjustment{};
	}

	default:
	{
		return wholeRow;
	}
	}
}
