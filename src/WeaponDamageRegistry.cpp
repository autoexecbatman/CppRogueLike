// WeaponDamageRegistry.cpp
#include <string>
#include <string_view>
#include <unordered_map>

#include "ItemEnhancements.h"
#include "DamageInfo.h"
#include "WeaponDamageRegistry.h"

const std::unordered_map<std::string, DamageInfo> WeaponDamageRegistry::weaponDamageMap =
	WeaponDamageRegistry::create_weapon_damage_map();

std::unordered_map<std::string, DamageInfo> WeaponDamageRegistry::create_weapon_damage_map()
{
	return {
		// Melee Weapons - AD&D 2e damage values
		{ "dagger", DamageValues::Dagger() },
		{ "short_sword", DamageValues::ShortSword() },
		{ "long_sword", DamageValues::LongSword() },
		{ "bastard_sword", DamageValues::LongSword() },
		{ "two_handed_sword", DamageValues::GreatSword() },
		{ "great_sword", DamageValues::GreatSword() },
		{ "scimitar", DamageValues::LongSword() },
		{ "rapier", DamageValues::ShortSword() },
		{ "hand_axe", DamageValues::Dagger() },
		{ "battle_axe", DamageValues::BattleAxe() },
		{ "great_axe", { "1d12", DamageType::PHYSICAL } },
		{ "war_hammer", DamageValues::WarHammer() },
		{ "mace", { "1d6+1", DamageType::PHYSICAL } },
		{ "morning_star", { "2d4", DamageType::PHYSICAL } },
		{ "flail", { "1d6+1", DamageType::PHYSICAL } },
		{ "club", { "1d6", DamageType::PHYSICAL } },
		{ "quarterstaff", DamageValues::Staff() },
		{ "staff", DamageValues::Staff() },

		// Ranged Weapons
		{ "short_bow", { "1d6", DamageType::PHYSICAL } },
		{ "long_bow", DamageValues::LongBow() },
		{ "composite_bow", { "1d6", DamageType::PHYSICAL } },
		{ "light_crossbow", { "1d8", DamageType::PHYSICAL } },
		{ "heavy_crossbow", { "1d10", DamageType::PHYSICAL } },
		{ "sling", { "1d4", DamageType::PHYSICAL } },
	};
}

DamageInfo WeaponDamageRegistry::get_damage_info(std::string_view weaponKey) noexcept
{
	if (weaponDamageMap.contains(std::string{ weaponKey }))
	{
		return weaponDamageMap.at(std::string{ weaponKey });
	}
	return get_unarmed_damage_info();
}

DamageInfo WeaponDamageRegistry::get_enhanced_damage_info(std::string_view weaponKey, const ItemEnhancement* enhancement) noexcept
{
	DamageInfo baseDamage = get_damage_info(weaponKey);

	if (enhancement && enhancement->damageBonus != 0)
	{
		return baseDamage.with_enhancement(enhancement->damageBonus);
	}

	return baseDamage;
}

std::string WeaponDamageRegistry::get_damage_roll(std::string_view weaponKey) noexcept
{
	return get_damage_info(weaponKey).displayRoll;
}

bool WeaponDamageRegistry::is_registered(std::string_view weaponKey) noexcept
{
	return weaponDamageMap.contains(std::string{ weaponKey });
}
