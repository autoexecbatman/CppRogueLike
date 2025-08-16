#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "../Game.h"
#include "Weapons.h"

using json = nlohmann::json;

void from_json(const json& j, Weapons& w) 
{
	j.at("name").get_to(w.name);
	j.at("type").get_to(w.type);
	j.at("damageRoll").get_to(w.damageRoll);
	
	// Load hand requirement with default to ONE_HANDED
	if (j.contains("handRequirement"))
	{
		std::string handReqStr;
		j.at("handRequirement").get_to(handReqStr);
		
		if (handReqStr == "TWO_HANDED")
		{
			w.handRequirement = HandRequirement::TWO_HANDED;
		}
		else if (handReqStr == "OFF_HAND_ONLY")
		{
			w.handRequirement = HandRequirement::OFF_HAND_ONLY;
		}
		else
		{
			w.handRequirement = HandRequirement::ONE_HANDED;
		}
	}
	else
	{
		w.handRequirement = HandRequirement::ONE_HANDED;
	}
	
	// Load weapon size with default to MEDIUM
	if (j.contains("weaponSize"))
	{
		std::string sizeStr;
		j.at("weaponSize").get_to(sizeStr);
		
		if (sizeStr == "TINY")
		{
			w.weaponSize = WeaponSize::TINY;
		}
		else if (sizeStr == "SMALL")
		{
			w.weaponSize = WeaponSize::SMALL;
		}
		else if (sizeStr == "LARGE")
		{
			w.weaponSize = WeaponSize::LARGE;
		}
		else if (sizeStr == "HUGE")
		{
			w.weaponSize = WeaponSize::GIANT;
		}
		else
		{
			w.weaponSize = WeaponSize::MEDIUM; // Default
		}
	}
	else
	{
		w.weaponSize = WeaponSize::MEDIUM; // Default
	}
	
	j.at("hitBonusRange").get_to(w.hitBonusRange);
	j.at("damageBonusRange").get_to(w.damageBonusRange);
	j.at("specialProperties").get_to(w.specialProperties);
	
	// enhancementLevel defaults to 0 if not specified in JSON
	if (j.contains("enhancementLevel"))
	{
		j.at("enhancementLevel").get_to(w.enhancementLevel);
	}
	else
	{
		w.enhancementLevel = 0;
	}
}

// Two-handed weapon methods implementation
std::string Weapons::get_damage_roll(bool twoHanded) const noexcept
{
	// Always use regular damage roll since VERSATILE removed
	return damageRoll;
}

std::vector<Weapons> loadWeapons()
{
	std::ifstream i("./weapons.json");

	if (!i.is_open()) {
		game.log("Error: Unable to open weapons.json. Error " + std::to_string(errno));
		throw std::runtime_error("Error: Unable to open weapons.json. Error " + std::to_string(errno));
	}

	json j;
	try {
		i >> j;
	}
	catch (const json::parse_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to parse weapons.json. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}

	try {
		std::vector<Weapons> weaponsChart = j.get<std::vector<Weapons>>();
		return weaponsChart;
	}
	catch (const json::type_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to convert JSON to vector<Weapons>. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}
}

// Enhanced bonus calculation methods
int Weapons::get_min_hit_bonus() const
{
	if (hitBonusRange.empty()) return enhancementLevel;
return hitBonusRange[0] + enhancementLevel;
}

int Weapons::get_max_hit_bonus() const
{
if (hitBonusRange.empty()) return enhancementLevel;
return hitBonusRange[1] + enhancementLevel;
}

int Weapons::get_min_damage_bonus() const
{
if (damageBonusRange.empty()) return enhancementLevel;
return damageBonusRange[0] + enhancementLevel;
}

int Weapons::get_max_damage_bonus() const
{
if (damageBonusRange.empty()) return enhancementLevel;
return damageBonusRange[1] + enhancementLevel;
}

std::string Weapons::get_display_name() const
{
if (enhancementLevel > 0)
	{
		return name + " +" + std::to_string(enhancementLevel);
	}
	return name;
}

// Enhancement methods
void Weapons::set_enhancement_level(int level)
{
	enhancementLevel = (level >= 0) ? level : 0;
}

void Weapons::enhance_weapon(int levels)
{
	if (levels > 0)
	{
		enhancementLevel += levels;
	}
}

void Weapons::print_chart()
{
	std::vector<Weapons> weaponChart = loadWeapons();

	for (const auto& weapon : weaponChart)
	{
		std::cout << "name: " << weapon.get_display_name() << std::endl;
		std::cout << "type: " << weapon.type << std::endl;
		std::cout << "damageRoll: " << weapon.damageRoll << std::endl;
		std::cout << "hitBonusRange: " << weapon.get_min_hit_bonus() << "-" << weapon.get_max_hit_bonus() << std::endl;
		std::cout << "damageBonusRange: " << weapon.get_min_damage_bonus() << "-" << weapon.get_max_damage_bonus() << std::endl;
		std::cout << "enhancementLevel: " << weapon.enhancementLevel << std::endl;
		std::cout << "specialProperties: ";
		for (const auto& i : weapon.specialProperties)
		{
			std::cout << i << ",";
		}
		std::cout << std::endl;
	}

}

// AD&D 2e Two-Weapon Fighting validation methods
bool Weapons::is_compatible_off_hand(const Weapons& mainHandWeapon) const noexcept
{
	// Off-hand weapon must be smaller than or equal to main hand weapon
	// AND must be SMALL or TINY size
	return can_be_off_hand() && (weaponSize <= mainHandWeapon.weaponSize);
}

bool Weapons::can_dual_wield(const Weapons& mainHand, const Weapons& offHand) noexcept
{
	// AD&D 2e Two-Weapon Fighting Rules:
	// 1. Main hand weapon must be one-handed and MEDIUM or smaller
	// 2. Off-hand weapon must be SMALL or TINY
	// 3. Off-hand weapon must be smaller than or equal to main hand
	// 4. Both weapons must be one-handed
	
	if (!mainHand.can_be_main_hand() || !offHand.can_be_off_hand())
	{
		return false;
	}
	
	// Off-hand must be smaller than or equal to main hand
	if (offHand.weaponSize > mainHand.weaponSize)
	{
		return false;
	}
	
	return true;
}