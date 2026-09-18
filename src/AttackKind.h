#pragma once

// Which way an attack is being made. The attacker's equipment cannot answer this:
// somebody carrying a bow still swings the sword in their hand when they walk into
// a goblin, and the same creature loosing an arrow at range is doing something
// else. Only the caller knows which one it started, so it says so.
enum class AttackKind
{
	MELEE,  // Struck at touching range with whatever is in hand
	RANGED, // Loosed from the missile slot at a target further off
};
