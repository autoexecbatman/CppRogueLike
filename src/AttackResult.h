#pragma once

// file: AttackResult.h
//
// What one attack did, told back to whoever made it. A bite that carries poison needs it:
// poison "must be injected into the bloodstream by bite or sting" (Dungeon Master's Guide,
// PDF page 736 of the 2e archive), so a bite that never landed injects nothing.
//
// Usage:
//
//   const AttackResult::AttackResult result =
//       owner.attacker->attack(target, AttackKind::AttackKind::MELEE, ctx);
//   // the blow connected; poison follows it
//   if (result == AttackResult::AttackResult::LANDED) { ... }
namespace AttackResult
{
	enum class AttackResult
	{
		// the roll beat the target's armour class and the damage was applied
		LANDED,
		// the roll to hit fell short
		MISSED,
		// nothing was rolled: a trade, a dead target, a ward, a bow too heavy
		// to draw
		PREVENTED,
	};
} // namespace AttackResult
