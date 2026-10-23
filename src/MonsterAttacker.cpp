#include "MonsterAttacker.h"

#include <cassert>

#include "AttackKind.h"
#include "Creature.h"
#include "GameContext.h"
#include "Item.h"

MonsterAttacker::MonsterAttacker(Creature& owner, const DamageInfo& damage) :
	Attacker(damage),
	owner(owner)
{
}

AttackResult MonsterAttacker::attack(
	Creature& target,
	AttackKind kind,
	GameContext& ctx)
{
	// A monster using a weapon does the weapon's damage (Monstrous Manual, PDF page 1083).
	if (kind == AttackKind::RANGED)
	{
		const Item* missileWeapon =
			owner.get_equipped_item(weapon_slot_for(kind));
		assert(
			missileWeapon &&
			"MonsterAttacker::attack: a ranged attack with no missile weapon");
		return perform_single_attack(
			owner,
			target,
			weapon_damage(missileWeapon),
			weapon_hit_modifier(missileWeapon),
			missileWeapon->get_name(),
			kind,
			ctx);
	}

	return perform_single_attack(
		owner,
		target,
		get_damage_info(),
		0,
		owner.get_attack_name(),
		kind,
		ctx);
}
