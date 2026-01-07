#pragma once

#include <vector>
#include <string>

enum class HandRequirement
{
	ONE_HANDED,     // Dagger, short sword, longsword, battle axe, war hammer, staff
	TWO_HANDED,     // Greatsword, great axe, longbow
	OFF_HAND_ONLY   // Shield, buckler
};

// AD&D 2e Weapon Size Categories for Two-Weapon Fighting
enum class WeaponSize
{
	TINY,     // Dagger, dart - can always be off-hand
	SMALL,    // Short sword, hand axe, club - can be off-hand vs MEDIUM+ main hand
	MEDIUM,   // Long sword, battle axe, mace - main hand weapon
	LARGE,    // Two-handed sword, halberd - cannot dual wield
	GIANT      // Giant weapons - cannot dual wield
};

// LEGACY: Weapons struct maintained for data loading compatibility only
// Combat system uses WeaponDamageRegistry + DamageInfo exclusively
// DO NOT EXPAND - Enhancement system removed, dual-wield removed
// Usage limited to: DataManager loading, ItemCreator template data
struct Weapons
{
	std::string name{};
	std::string type{};
	std::string damageRoll{};
	std::string damageRollTwoHanded{}; // For versatile weapons when used two-handed
	HandRequirement handRequirement{HandRequirement::ONE_HANDED};
	WeaponSize weaponSize{WeaponSize::MEDIUM}; // AD&D 2e size category
	std::vector<int> hitBonusRange{};
	std::vector<int> damageBonusRange{};
	std::vector<std::string> specialProperties{};
	int enhancementLevel{0};

	// NOTE: Enhancement system removed - was never integrated with combat

	// Two-handed weapon methods
	bool is_two_handed() const noexcept { return handRequirement == HandRequirement::TWO_HANDED; }
	bool can_use_one_handed() const noexcept { return handRequirement == HandRequirement::ONE_HANDED; }
	bool can_use_two_handed() const noexcept { return handRequirement == HandRequirement::TWO_HANDED; }
	std::string get_damage_roll(bool twoHanded = false) const noexcept;
	
	// NOTE: Complex dual-wield logic removed - was never integrated with combat
};
