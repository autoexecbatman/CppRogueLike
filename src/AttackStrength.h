#pragma once

// file: AttackStrength.h
//
// How much of a Strength row (Player's Handbook Table 1) an attack takes, decided by
// what makes the attack. The book, PDF page 181: Strength "is always applied to melees
// and attacks with hurled missile weapons"; a bow takes a bonus only when specially
// made for its wielder and a penalty always; a crossbow never takes either. A sling is
// a missile the rule does not name and takes neither - the owner's reading, 2026-09-19.
//
// Usage:
//
//   const StrengthAttributes strong = dataManager.strength_for(18, 100);   // 18/00: +3 hit, +6 damage
//   const StrengthAttributes weak = dataManager.strength_for(3, 0);        // 3: -3 hit, -1 damage
//   AttackStrength::adjustment(strong, AttackKind::MELEE, nullptr);        // -> { 3, 6 }, a swing takes it all
//   AttackStrength::adjustment(strong, AttackKind::RANGED, &longBow);      // -> { 0, 0 }, no bonus for a bow
//   AttackStrength::adjustment(weak, AttackKind::RANGED, &longBow);        // -> { -3, -1 }, the penalty stays
//   AttackStrength::adjustment(weak, AttackKind::RANGED, &crossbow);       // -> { 0, 0 }, a machine draws it

#include "AttackKind.h"
#include "StrengthAttributes.h"

class Item;

namespace AttackStrength
{

// What Strength adds to an attack's roll to hit and to its damage.
struct Adjustment
{
	int hit{ 0 };
	int damage{ 0 };
};

// The part of the attacker's Strength row this attack takes. missileWeapon is what is
// fired on a ranged attack; with none, the attack is natural or hurled and takes the
// whole row, as a swing does.
//
// Example, a Strength 3 archer:
//   adjustment(weak, AttackKind::RANGED, &shortBow);   // -> { -3, -1 }
//   adjustment(weak, AttackKind::RANGED, &sling);      // -> { 0, 0 }
[[nodiscard]] Adjustment adjustment(const StrengthAttributes& row, AttackKind kind, const Item* missileWeapon);

} // namespace AttackStrength
