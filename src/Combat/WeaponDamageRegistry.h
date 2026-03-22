#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "DamageInfo.h"

// Forward declarations
struct ItemEnhancement;

class WeaponDamageRegistry
{
public:
	// Get damage info for weapon key
	static DamageInfo get_damage_info(std::string_view weaponKey) noexcept;

	// Get enhanced damage info with weapon modifiers
	static DamageInfo get_enhanced_damage_info(std::string_view weaponKey, const ItemEnhancement* enhancement) noexcept;

	// Get damage roll string for display
	static std::string get_damage_roll(std::string_view weaponKey) noexcept;

	// Check if weapon key is registered
	static bool is_registered(std::string_view weaponKey) noexcept;

	// Get default unarmed damage
	static DamageInfo get_unarmed_damage_info() noexcept { return DamageValues::Unarmed(); }
	static std::string get_unarmed_damage() noexcept { return "1d2"; }

private:
	static const std::unordered_map<std::string, DamageInfo> weapon_damage_map;

	static std::unordered_map<std::string, DamageInfo> create_weapon_damage_map();
};
