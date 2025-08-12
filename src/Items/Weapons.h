#pragma once

#include <vector>
#include <string>

enum class HandRequirement
{
	ONE_HANDED,     // Dagger, short sword
	TWO_HANDED,     // Greatsword, longbow
	VERSATILE,      // Longsword, battle axe (can be 1H or 2H)
	OFF_HAND_ONLY   // Shield, buckler
};

struct Weapons
{
	std::string name{};
	std::string type{};
	std::string damageRoll{};
	std::string damageRollTwoHanded{}; // For versatile weapons when used two-handed
	HandRequirement handRequirement{HandRequirement::ONE_HANDED};
	std::vector<int> hitBonusRange{};
	std::vector<int> damageBonusRange{};
	std::vector<std::string> specialProperties{};
	int enhancementLevel{0};

	// Enhanced bonus calculation methods
	int getMinHitBonus() const;
	int getMaxHitBonus() const;
	int getMinDamageBonus() const;
	int getMaxDamageBonus() const;
	std::string getDisplayName() const;

	// Enhancement methods
	void setEnhancementLevel(int level);
	void enhanceWeapon(int levels = 1);

	// Two-handed weapon methods
	bool isTwoHanded() const noexcept { return handRequirement == HandRequirement::TWO_HANDED; }
	bool isVersatile() const noexcept { return handRequirement == HandRequirement::VERSATILE; }
	bool canUseOneHanded() const noexcept { return handRequirement == HandRequirement::ONE_HANDED || handRequirement == HandRequirement::VERSATILE; }
	bool canUseTwoHanded() const noexcept { return handRequirement == HandRequirement::TWO_HANDED || handRequirement == HandRequirement::VERSATILE; }
	std::string getDamageRoll(bool twoHanded = false) const noexcept;

	void print_chart();
};

std::vector<Weapons> loadWeapons();
