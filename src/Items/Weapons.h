#pragma once

#include <vector>
#include <string>

struct Weapons
{
	std::string name{};
	std::string type{};
	std::string damageRoll{};
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

	void print_chart();
};

std::vector<Weapons> loadWeapons();
