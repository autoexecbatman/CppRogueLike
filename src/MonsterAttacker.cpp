#include "MonsterAttacker.h"

#include "Creature.h"
#include "GameContext.h"

MonsterAttacker::MonsterAttacker(Creature& owner, const DamageInfo& damage)
	: Attacker(damage), owner(owner) {}

AttackResult MonsterAttacker::attack(Creature& target, AttackKind kind, GameContext& ctx)
{
	return perform_single_attack(owner, target, get_damage_info(), 0, owner.get_attack_name(), kind, ctx);
}
