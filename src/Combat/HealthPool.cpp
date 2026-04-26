// file: HealthPool.cpp
#include <algorithm>
#include <format>

#include "HealthPool.h"
#include "DamageResolver.h"
#include "DamageInfo.h"
#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Systems/FloatingTextSystem.h"

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

	if (ctx.floatingText)
	{
		const bool hitPlayer = (&owner == ctx.player);
		const unsigned char r = hitPlayer ? 255 : 255;
		const unsigned char g = hitPlayer ? 80 : 220;
		const unsigned char b = hitPlayer ? 80 : 50;
		ctx.floatingText->spawn_damage(owner.position.x, owner.position.y, actualDamage, r, g, b);
	}

	return actualDamage;
}

int HealthPool::heal(int hpToHeal)
{
	const int currentHp = hp;
	const int newHp = std::min(currentHp + hpToHeal, hpMax);
	const int actualHealed = newHp - currentHp;

	hp = newHp;

	return actualHealed;
}

// end of file: HealthPool.cpp
