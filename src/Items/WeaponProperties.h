#pragma once

// AD&D 2e Weapon Properties
// Extracted from deprecated Weapons.h - these enums are still actively used

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
	GIANT     // Giant weapons - cannot dual wield
};
