#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "WisdomAttributes.h"
#include "../Game.h"

using json = nlohmann::json;

void from_json(const json& j, WisdomAttributes& p) {
	j.at("Wis").get_to(p.Wis);
	j.at("MagicalDefenseAdj").get_to(p.MagicalDefenseAdj);
	j.at("BonusSpells").get_to(p.BonusSpells);
	j.at("ChanceOfSpellFailure").get_to(p.ChanceOfSpellFailure);
	j.at("SpellImmunity").get_to(p.SpellImmunity);
}

std::vector<WisdomAttributes> loadWisdomAttributes() {
	std::ifstream i("wisdom.json");

	if (!i.is_open()) {
		game.log("Error: Unable to open wisdom.json. Error " + std::to_string(errno));
		throw std::runtime_error("Error: Unable to open wisdom.json. Error " + std::to_string(errno));
	}

	json j;
	try {
		i >> j;
	}
	catch (const json::parse_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to parse wisdom.json. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}

	try {
		std::vector<WisdomAttributes> wisdomChart = j.get<std::vector<WisdomAttributes>>();
		return wisdomChart;
	}
	catch (const json::type_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to convert JSON to vector<WisdomAttributes>. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}
}