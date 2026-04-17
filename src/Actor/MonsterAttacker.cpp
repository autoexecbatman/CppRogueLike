#include "MonsterAttacker.h"

#include "../Actor/Creature.h"
#include "../Core/GameContext.h"

MonsterAttacker::MonsterAttacker(Creature& owner, const DamageInfo& damage)
	: Attacker(damage), owner(owner) {}

void MonsterAttacker::attack(Creature& target, GameContext& ctx)
{
	perform_single_attack(owner, target, get_damage_info(), 0, owner.get_weapon_equipped(), ctx);
}
