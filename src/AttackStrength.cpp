// file: AttackStrength.cpp
#include "AttackStrength.h"

#include <algorithm>

#include "DataManager.h"
#include "Item.h"
#include "ItemClassification.h"
#include "Pickable.h"
#include "StrengthAttributes.h"

AttackStrength::Adjustment AttackStrength::adjustment(
	const DataManager& dataManager,
	int strength,
	int exceptionalStrength,
	AttackKind kind,
	const Item* missileWeapon)
{
	const StrengthAttributes row = dataManager.strength_for(strength, exceptionalStrength);

	// A swing, and a missile thrown by hand or by nature, carries the arm's whole strength.
	const Adjustment wholeRow{ row.hitProb, row.dmgAdj };
	if (kind == AttackKind::MELEE || missileWeapon == nullptr)
	{
		return wholeRow;
	}

	switch (missileWeapon->itemClass)
	{
	case ItemClass::BOW:
	{
		// A bow made for an arm gives that arm's row; can_draw stops a weaker arm before here.
		if (const int rating = strength_rating_of(*missileWeapon); rating > 0)
		{
			const StrengthAttributes madeFor = dataManager.strength_for(rating, 0);
			return Adjustment{ madeFor.hitProb, madeFor.dmgAdj };
		}

		// An ordinary bow: a weak arm cannot draw it fully, and a strong one gains nothing.
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
