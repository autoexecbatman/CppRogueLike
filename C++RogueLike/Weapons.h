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

	void print_chart();
};

std::vector<Weapons> loadWeapons();
