// WeaponDamageRegistry.cpp
#include <cassert>
#include <string>
#include <string_view>
#include <unordered_map>

#include "DamageInfo.h"
#include "ItemEnhancements.h"
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
		{ "light_crossbow", { "1d4", DamageType::PHYSICAL } },
		{ "heavy_crossbow", { "1d4+1", DamageType::PHYSICAL } },
		{ "sling", { "1d4", DamageType::PHYSICAL } },
	};
}

DamageInfo WeaponDamageRegistry::get_damage_info(std::string_view weaponKey) noexcept
{
	const auto found = weaponDamageMap.find(std::string{ weaponKey });

	// Every caller asks is_weapon() first and a test walks the data for a weapon this
	// table has no row for, so a key that is not here means the two disagree rather
	// than that the player is holding something odd.
	assert(found != weaponDamageMap.end() && "get_damage_info: asked for a key the weapon table has no damage of its own for");

	// What a Release build draws rather than crashing on a path the character sheet
	// and the inventory both call to paint themselves.
	if (found == weaponDamageMap.end())
	{
		return get_unarmed_damage_info();
	}

	return found->second;
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
