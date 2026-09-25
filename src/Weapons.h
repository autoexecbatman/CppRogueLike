#pragma once

#include <algorithm>
#include <array>

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

enum class HandRequirement
{
	ONE_HANDED, // Dagger, short sword, longsword, battle axe, war hammer, staff
	TWO_HANDED, // Greatsword, great axe, longbow
	OFF_HAND_ONLY // Shield, buckler
};

// Every HandRequirement, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<HandRequirement, 3> ALL_HAND_REQUIREMENT = {
	HandRequirement::ONE_HANDED,
	HandRequirement::TWO_HANDED,
	HandRequirement::OFF_HAND_ONLY,
};

// The next HandRequirement in that order, wrapping at the end.
//
// Example:
//   next_hand_requirement(HandRequirement::ONE_HANDED);   // -> HandRequirement::TWO_HANDED
[[nodiscard]] inline HandRequirement next_hand_requirement(HandRequirement value)
{
	const auto found = std::ranges::find(ALL_HAND_REQUIREMENT, value);
	if (found == ALL_HAND_REQUIREMENT.end() || found + 1 == ALL_HAND_REQUIREMENT.end())
	{
		return ALL_HAND_REQUIREMENT.front();
	}
	return *(found + 1);
}

// AD&D 2e Weapon Size Categories for Two-Weapon Fighting
enum class WeaponSize
{
	TINY, // Dagger, dart - can always be off-hand
	SMALL, // Short sword, hand axe, club - can be off-hand vs MEDIUM+ main hand
	MEDIUM, // Long sword, battle axe, mace - main hand weapon
	LARGE, // Two-handed sword, halberd - cannot dual wield
	GIANT // Giant weapons - cannot dual wield
};

// Every WeaponSize, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<WeaponSize, 5> ALL_WEAPON_SIZE = {
	WeaponSize::TINY,
	WeaponSize::SMALL,
	WeaponSize::MEDIUM,
	WeaponSize::LARGE,
	WeaponSize::GIANT,
};

// The next WeaponSize in that order, wrapping at the end.
//
// Example:
//   next_weapon_size(WeaponSize::TINY);   // -> WeaponSize::SMALL
[[nodiscard]] inline WeaponSize next_weapon_size(WeaponSize value)
{
	const auto found = std::ranges::find(ALL_WEAPON_SIZE, value);
	if (found == ALL_WEAPON_SIZE.end() || found + 1 == ALL_WEAPON_SIZE.end())
	{
		return ALL_WEAPON_SIZE.front();
	}
	return *(found + 1);
}

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
	HandRequirement handRequirement{ HandRequirement::ONE_HANDED };
	WeaponSize weaponSize{ WeaponSize::MEDIUM }; // AD&D 2e size category
	std::vector<int> hitBonusRange{};
	std::vector<int> damageBonusRange{};
	std::vector<std::string> specialProperties{};
	int enhancementLevel{ 0 };

	// NOTE: Enhancement system removed - was never integrated with combat

	// Two-handed weapon methods
	bool is_two_handed() const noexcept { return handRequirement == HandRequirement::TWO_HANDED; }
	bool can_use_one_handed() const noexcept { return handRequirement == HandRequirement::ONE_HANDED; }
	bool can_use_two_handed() const noexcept { return handRequirement == HandRequirement::TWO_HANDED; }
	std::string get_damage_roll(bool twoHanded = false) const noexcept;

	// NOTE: Complex dual-wield logic removed - was never integrated with combat
};

inline std::string_view encode_hand_requirement(HandRequirement handRequirement)
{
	switch (handRequirement)
	{

	case HandRequirement::ONE_HANDED:
	{
		return "one_handed";
	}

	case HandRequirement::TWO_HANDED:
	{
		return "two_handed";
	}

	case HandRequirement::OFF_HAND_ONLY:
	{
		return "off_hand_only";
	}

	}

	return "one_handed";
}

inline HandRequirement parse_hand_requirement(std::string_view name)
{
	if (name == "one_handed")
	{
		return HandRequirement::ONE_HANDED;
	}
	if (name == "two_handed")
	{
		return HandRequirement::TWO_HANDED;
	}
	if (name == "off_hand_only")
	{
		return HandRequirement::OFF_HAND_ONLY;
	}

	throw std::runtime_error(std::format("unknown hand_requirement '{}'", name));
}

inline std::string_view encode_weapon_size(WeaponSize weaponSize)
{
	switch (weaponSize)
	{

	case WeaponSize::TINY:
	{
		return "tiny";
	}

	case WeaponSize::SMALL:
	{
		return "small";
	}

	case WeaponSize::MEDIUM:
	{
		return "medium";
	}

	case WeaponSize::LARGE:
	{
		return "large";
	}

	case WeaponSize::GIANT:
	{
		return "giant";
	}

	}

	return "medium";
}

inline WeaponSize parse_weapon_size(std::string_view name)
{
	if (name == "tiny")
	{
		return WeaponSize::TINY;
	}
	if (name == "small")
	{
		return WeaponSize::SMALL;
	}
	if (name == "medium")
	{
		return WeaponSize::MEDIUM;
	}
	if (name == "large")
	{
		return WeaponSize::LARGE;
	}
	if (name == "giant")
	{
		return WeaponSize::GIANT;
	}

	throw std::runtime_error(std::format("unknown weapon_size '{}'", name));
}
