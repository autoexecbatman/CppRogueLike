#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "DexterityAttributes.h"
#include "../Game.h"

using json = nlohmann::json;

void from_json(const json& j, DexterityAttributes& p) 
{
	j.at("Dex").get_to(p.Dex);
	j.at("ReactionAdj").get_to(p.ReactionAdj);
	j.at("MissileAttackAdj").get_to(p.MissileAttackAdj);
	j.at("DefensiveAdj").get_to(p.DefensiveAdj);
}

std::vector<DexterityAttributes> loadDexterityAttributes()
{
	std::ifstream i("dexterity.json");

	if (!i.is_open()) {
		game.log("Error: Unable to open dexterity.json. Error " + std::to_string(errno));
		throw std::runtime_error("Error: Unable to open dexterity.json. Error " + std::to_string(errno));
	}

	json j;
	try {
		i >> j;
	}
	catch (const json::parse_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to parse dexterity.json. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}

	try {
		std::vector<DexterityAttributes> dexterityChart = j.get<std::vector<DexterityAttributes>>();
		return dexterityChart;
	}
	catch (const json::type_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to convert JSON to vector<DexterityAttributes>. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}
}