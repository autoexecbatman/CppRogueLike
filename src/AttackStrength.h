#pragma once

// file: AttackStrength.h
//
// How much of a Strength row (Player's Handbook Table 1) an attack takes, decided by
// what makes the attack. The book, PDF page 181: Strength "is always applied to melees
// and attacks with hurled missile weapons"; a bow takes a bonus only when specially
// made for its wielder and a penalty always; a crossbow never takes either. A sling is
// a missile the rule does not name and takes neither - the owner's reading, 2026-09-19.
// A specially made bow gives the row of the Strength it is made for: the composite bow,
// made for 18, gives +1 and +2, the first Baldur's Gate's numbers. Gauntlets of ogre power
// worn with a girdle of giant strength add their own row to a blow struck or hurled, where
// the girdle takes no other bonus (Dungeon Master's Guide, PDF pages 964 and 966).
//
// Usage, for a fighter of 18/00 (+3 hit, +6 damage on Table 1) and a weakling of 3 (-3 and -1):
//
//   AttackStrength::adjustment(dataManager, fighter, AttackKind::MELEE, nullptr); // -> { 3, 6 }, a swing takes it all
//   AttackStrength::adjustment(dataManager, fighter, AttackKind::RANGED, &longBow); // -> { 0, 0 }, no bonus for a bow
//   AttackStrength::adjustment(dataManager, weakling, AttackKind::RANGED, &longBow); // -> { -3, -1 }, the penalty stays
//   AttackStrength::adjustment(dataManager, weakling, AttackKind::RANGED, &crossbow); // -> { 0, 0 }, a machine draws it
//   AttackStrength::adjustment(dataManager, fighter, AttackKind::RANGED, &composite); // -> { 1, 2 }, the row for 18
//
//   // In a girdle of hill giant strength (19: +3, +7) and gauntlets of ogre power:
//   AttackStrength::adjustment(dataManager, fighter, AttackKind::MELEE, nullptr); // -> { 6, 13 }

#include "AttackKind.h"

class Creature;
class DataManager;
class Item;

namespace AttackStrength
{

// What Strength adds to an attack's roll to hit and to its damage.
struct Adjustment
{
	int hit{ 0 };
	int damage{ 0 };
};

// The part of the attacker's Strength this attack takes, read from Table 1 at its score
// and percentile, with the row of ogre gauntlets it wears beside a girdle of giant
// strength. missileWeapon is what is fired on a ranged attack; with none, the attack is
// natural or hurled and takes the whole row, as a swing does.
//
// Example, a Strength 3 archer:
//   adjustment(dataManager, weakling, AttackKind::RANGED, &shortBow); // -> { -3, -1 }
//   adjustment(dataManager, weakling, AttackKind::RANGED, &sling); // -> { 0, 0 }
[[nodiscard]] Adjustment adjustment(
	const DataManager& dataManager,
	const Creature& attacker,
	AttackKind kind,
	const Item* missileWeapon);

} // namespace AttackStrength
