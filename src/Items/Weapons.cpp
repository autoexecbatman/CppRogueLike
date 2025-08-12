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
	
	// Load optional damageRollTwoHanded for versatile weapons
	if (j.contains("damageRollTwoHanded"))
	{
		j.at("damageRollTwoHanded").get_to(w.damageRollTwoHanded);
	}
	
	// Load hand requirement with default to ONE_HANDED
	if (j.contains("handRequirement"))
	{
		std::string handReqStr;
		j.at("handRequirement").get_to(handReqStr);
		
		if (handReqStr == "TWO_HANDED")
		{
			w.handRequirement = HandRequirement::TWO_HANDED;
		}
		else if (handReqStr == "VERSATILE")
		{
			w.handRequirement = HandRequirement::VERSATILE;
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
std::string Weapons::getDamageRoll(bool twoHanded) const noexcept
{
	// For versatile weapons, use two-handed damage if available and requested
	if (twoHanded && isVersatile() && !damageRollTwoHanded.empty())
	{
		return damageRollTwoHanded;
	}
	
	// For two-handed weapons, always use regular damage roll
	// For one-handed weapons, always use regular damage roll
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
int Weapons::getMinHitBonus() const
{
	if (hitBonusRange.empty()) return enhancementLevel;
return hitBonusRange[0] + enhancementLevel;
}

int Weapons::getMaxHitBonus() const
{
if (hitBonusRange.empty()) return enhancementLevel;
return hitBonusRange[1] + enhancementLevel;
}

int Weapons::getMinDamageBonus() const
{
if (damageBonusRange.empty()) return enhancementLevel;
return damageBonusRange[0] + enhancementLevel;
}

int Weapons::getMaxDamageBonus() const
{
if (damageBonusRange.empty()) return enhancementLevel;
return damageBonusRange[1] + enhancementLevel;
}

std::string Weapons::getDisplayName() const
{
if (enhancementLevel > 0)
	{
		return name + " +" + std::to_string(enhancementLevel);
	}
	return name;
}

// Enhancement methods
void Weapons::setEnhancementLevel(int level)
{
	enhancementLevel = (level >= 0) ? level : 0;
}

void Weapons::enhanceWeapon(int levels)
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
		std::cout << "name: " << weapon.getDisplayName() << std::endl;
		std::cout << "type: " << weapon.type << std::endl;
		std::cout << "damageRoll: " << weapon.damageRoll << std::endl;
		std::cout << "hitBonusRange: " << weapon.getMinHitBonus() << "-" << weapon.getMaxHitBonus() << std::endl;
		std::cout << "damageBonusRange: " << weapon.getMinDamageBonus() << "-" << weapon.getMaxDamageBonus() << std::endl;
		std::cout << "enhancementLevel: " << weapon.enhancementLevel << std::endl;
		std::cout << "specialProperties: ";
		for (const auto& i : weapon.specialProperties)
		{
			std::cout << i << ",";
		}
		std::cout << std::endl;
	}

}