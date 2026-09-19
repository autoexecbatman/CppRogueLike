// file: HealthPool.cpp
#include <algorithm>
#include <cassert>
#include <format>

#include "HealthPool.h"
#include "DamageResolver.h"
#include "DamageInfo.h"
#include "Creature.h"
#include "GameContext.h"

HealthPool::HealthPool(int hpMax)
	: hpBase(hpMax),
	  hpMax(hpMax),
	  hp(hpMax),
	  tempHp(0)
{
}

int HealthPool::take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType)
{
	if (damage <= 0)
	{
		return 0;
	}

	int actualDamage = DamageResolver::apply_resistances(damage, damageType, owner, ctx);

	const ShieldResult shieldResult = DamageResolver::apply_temp_hp_shield(actualDamage, tempHp);
	tempHp = shieldResult.tempHpAfterShield;
	actualDamage = shieldResult.damageAfterShield;

	if (actualDamage == 0)
	{
		return 0;
	}

	hp -= actualDamage;

	if (hp <= 0)
	{
		hp = 0;
	}

	// Fire and acid wounds are kept apart, never more than the damage the pool shows.
	if (damageType == DamageType::FIRE || damageType == DamageType::ACID)
	{
		unregenerableDamage = std::min(unregenerableDamage + actualDamage, hpMax - hp);
	}

	return actualDamage;
}

int HealthPool::heal(int hpToHeal)
{
	const int currentHp = hp;
	const int newHp = std::min(currentHp + hpToHeal, hpMax);
	const int actualHealed = newHp - currentHp;

	hp = newHp;
	unregenerableDamage -= std::min(unregenerableDamage, actualHealed);

	return actualHealed;
}

int HealthPool::regenerate(int points)
{
	// Regeneration heals wounds; it does not raise the dead.
	if (hp <= 0)
	{
		return 0;
	}
	assert(unregenerableDamage <= hpMax - hp && "HealthPool::regenerate: more fire and acid damage than damage");

	const int healed = std::min(points, hpMax - hp - unregenerableDamage);
	hp += healed;
	return healed;
}

// end of file: HealthPool.cpp
