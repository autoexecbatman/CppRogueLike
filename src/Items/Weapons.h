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

	// Enhanced bonus calculation methods
	int get_min_hit_bonus() const;
	int get_max_hit_bonus() const;
	int get_min_damage_bonus() const;
	int get_max_damage_bonus() const;
	std::string get_display_name() const;

	// Enhancement methods
	void set_enhancement_level(int level);
	void enhance_weapon(int levels = 1);

	// Two-handed weapon methods
	bool is_two_handed() const noexcept { return handRequirement == HandRequirement::TWO_HANDED; }
	bool can_use_one_handed() const noexcept { return handRequirement == HandRequirement::ONE_HANDED; }
	bool can_use_two_handed() const noexcept { return handRequirement == HandRequirement::TWO_HANDED; }
	std::string get_damage_roll(bool twoHanded = false) const noexcept;
	
	// AD&D 2e Two-Weapon Fighting methods
	bool can_be_main_hand() const noexcept { return weaponSize <= WeaponSize::MEDIUM && can_use_one_handed(); }
	bool can_be_off_hand() const noexcept { return weaponSize <= WeaponSize::SMALL && can_use_one_handed(); }
	bool is_compatible_off_hand(const Weapons& mainHandWeapon) const noexcept;
	static bool can_dual_wield(const Weapons& mainHand, const Weapons& offHand) noexcept;

	void print_chart();
};

std::vector<Weapons> loadWeapons();
