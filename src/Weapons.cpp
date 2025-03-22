#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "Game.h"
#include "Weapons.h"

using json = nlohmann::json;

void from_json(const json& j, Weapons& w) 
{
	j.at("name").get_to(w.name);
	j.at("type").get_to(w.type);
	j.at("damageRoll").get_to(w.damageRoll);
	j.at("hitBonusRange").get_to(w.hitBonusRange);
	j.at("damageBonusRange").get_to(w.damageBonusRange);
	j.at("specialProperties").get_to(w.specialProperties);
}

std::vector<Weapons> loadWeapons()
{
	std::ifstream i("weapons.json");

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

void Weapons::print_chart()
{
	std::vector<Weapons> weaponChart = loadWeapons();

	for (const auto& weapon : weaponChart)
	{
		std::cout << "name: " << weapon.name << std::endl;
		std::cout << "type: " << weapon.type << std::endl;
		std::cout << "damageRoll: " << weapon.damageRoll << std::endl;
		std::cout << "hitBonusRange: ";
		for (const auto& i : weapon.hitBonusRange)
		{
			std::cout << i << ",";
		}
		std::cout << std::endl;
		std::cout << "damageBonusRange: ";
		for (const auto& i : weapon.damageBonusRange)
		{
			std::cout << i << ",";
		}
		std::cout << std::endl;
		std::cout << "specialProperties: ";
		for (const auto& i : weapon.specialProperties)
		{
			std::cout << i << ",";
		}
		std::cout << std::endl;
	}

}