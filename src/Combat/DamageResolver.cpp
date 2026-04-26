#include <algorithm>
#include <format>
#include <unordered_map>

#include "DamageResolver.h"
#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Systems/BuffSystem.h"
#include "../Systems/BuffType.h"
#include "../Systems/MessageSystem.h"

// OCP: Data-driven damage resistance mapping
static const std::unordered_map<DamageType, BuffType> damageResistanceBuffs = {
	{ DamageType::FIRE, BuffType::FIRE_RESISTANCE },
	{ DamageType::COLD, BuffType::COLD_RESISTANCE },
	{ DamageType::LIGHTNING, BuffType::LIGHTNING_RESISTANCE },
	{ DamageType::POISON, BuffType::POISON_RESISTANCE },
	// DamageType::PHYSICAL, ACID, MAGIC have no resistance buffs
};

// OCP: Data-driven damage type names for logging
static const std::unordered_map<DamageType, std::string_view> damageTypeNames = {
	{ DamageType::PHYSICAL, "physical" },
	{ DamageType::FIRE, "fire" },
	{ DamageType::COLD, "cold" },
	{ DamageType::LIGHTNING, "lightning" },
	{ DamageType::POISON, "poison" },
	{ DamageType::ACID, "acid" },
	{ DamageType::MAGIC, "magic" },
};

int DamageResolver::apply_resistances(
	int damage,
	DamageType damageType,
	const Creature& owner,
	GameContext& ctx)
{
	// AD&D 2e: Apply resistance based on damage type (data-driven, OCP compliant)
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

	const std::string_view typeName = damageTypeNames.contains(damageType)
		? damageTypeNames.at(damageType)
		: "unknown";

	ctx.messageSystem->log(std::format(
		"You resisted {} {} damage! ({}% resistance, {} -> {})",
		damageReduced,
		typeName,
		resistancePercent,
		originalDamage,
		damage));

	return damage;
}

int DamageResolver::apply_temp_hp_shield(int damage, int& tempHp)
{
	// AD&D 2e: Temp HP absorbs damage first
	if (tempHp <= 0)
	{
		return damage;
	}

	const int tempAbsorbed = std::min(damage, tempHp);
	tempHp -= tempAbsorbed;
	damage -= tempAbsorbed;

	return std::max(0, damage);
}
