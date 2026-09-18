#include <algorithm>
#include <format>
#include <unordered_map>

#include "DamageResolver.h"
#include "DamageInfo.h"
#include "Creature.h"
#include "GameContext.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "MessageSystem.h"

namespace
{

// The buff that resists each damage type. Types absent from the map have no
// resistance buff: physical, acid and magic.
const std::unordered_map<DamageType, BuffType> damageResistanceBuffs = {
	{ DamageType::FIRE, BuffType::FIRE_RESISTANCE },
	{ DamageType::COLD, BuffType::COLD_RESISTANCE },
	{ DamageType::LIGHTNING, BuffType::LIGHTNING_RESISTANCE },
	{ DamageType::POISON, BuffType::POISON_RESISTANCE },
};


// Per ring of resistance strength, from the two Dungeon Master's Guide
// entries: fire is -2 a die and +4 on the save, cold is -1 and +2.
constexpr int FIRE_PER_DIE = 2;
constexpr int FIRE_SAVE_BONUS = 4;
constexpr int COLD_PER_DIE = 1;
constexpr int COLD_SAVE_BONUS = 2;

} // namespace

int DamageResolver::apply_resistances(
	int damage,
	DamageType damageType,
	const Creature& owner,
	GameContext& ctx)
{
	// Fire and cold are resisted at the source, on the dice and the save, by
	// every producer there is: SpellSystem::burn_with_fireball for the fireball
	// spell and scroll, and Attacker::perform_single_attack for every typed
	// natural attack - chimera, dragon, fire wolf, pit fiend, ice wolf. A new
	// producer of fire or cold rolls its dice through reduce_dice and joins this
	// list. A fire or cold total can only arrive here as a ResistedDamage, because
	// the plain-integer damage path refuses those types.
	if (damageType == DamageType::FIRE || damageType == DamageType::COLD)
	{
		return damage;
	}
	if (!damageResistanceBuffs.contains(damageType))
	{
		return damage;
	}

	const BuffType resistanceBuff = damageResistanceBuffs.at(damageType);
	if (!ctx.buffSystem->has_buff(owner, resistanceBuff))
	{
		return damage;
	}
	const int resistancePercent = ctx.buffSystem->get_buff_value(owner, resistanceBuff);
	if (resistancePercent <= 0)
	{
		return damage;
	}

	const int originalDamage = damage;
	const int damageReduced = (damage * resistancePercent) / 100;
	damage -= damageReduced;
	damage = std::max(0, damage);

	ctx.messageSystem->log(std::format(
		"You resisted {} {} damage! ({}% resistance, {} -> {})",
		damageReduced,
		damage_type_name(damageType),
		resistancePercent,
		originalDamage,
		damage));

	return damage;
}

int DamageResolver::resistance_strength(DamageType damageType, const Creature& owner, GameContext& ctx)
{
	if (damageType != DamageType::FIRE && damageType != DamageType::COLD)
	{
		return 0;
	}
	// A potion is a ring's worth for its duration; the buff value is its strength.
	const BuffType resistanceBuff = damageResistanceBuffs.at(damageType);
	const int drunk = ctx.buffSystem->has_buff(owner, resistanceBuff)
		? ctx.buffSystem->get_buff_value(owner, resistanceBuff)
		: 0;
	return std::max(drunk, owner.worn_resistance_strength(damageType));
}

int DamageResolver::save_bonus_against(DamageType damageType, const Creature& owner, GameContext& ctx)
{
	const int strength = resistance_strength(damageType, owner, ctx);
	switch (damageType)
	{
	case DamageType::FIRE:
	{
		return FIRE_SAVE_BONUS * strength;
	}
	case DamageType::COLD:
	{
		return COLD_SAVE_BONUS * strength;
	}
	default:
	{
		return 0;
	}
	}
}

DamageResolver::ResistedDamage DamageResolver::reduce_dice(const std::vector<int>& dice, DamageType damageType, int strength)
{
	int perDie = 0;
	switch (damageType)
	{
	case DamageType::FIRE:
	{
		perDie = FIRE_PER_DIE * strength;
		break;
	}
	case DamageType::COLD:
	{
		perDie = COLD_PER_DIE * strength;
		break;
	}
	default:
	{
		break;
	}
	}
	int total = 0;
	for (const int die : dice)
	{
		// "each die is never less than 1 in any event"
		total += std::max(1, die - perDie);
	}
	return ResistedDamage{ damageType, total };
}

ShieldResult DamageResolver::apply_temp_hp_shield(int damage, int tempHp)
{
	// AD&D 2e: Temp HP absorbs damage first (pure calculation)
	if (tempHp <= 0)
	{
		return { damage, 0 };
	}

	const int tempAbsorbed = std::min(damage, tempHp);
	const int damageAfterShield = std::max(0, damage - tempAbsorbed);
	const int newTempHp = tempHp - tempAbsorbed;

	return { damageAfterShield, newTempHp };
}
