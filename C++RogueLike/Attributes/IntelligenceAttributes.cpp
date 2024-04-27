#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "Game.h"
#include "IntelligenceAttributes.h"

using json = nlohmann::json;

void from_json(const json& j, IntelligenceAttributes& p) {
	j.at("Int").get_to(p.Int);
	j.at("NumberOfLanguages").get_to(p.NumberOfLanguages);
	j.at("SpellLevel").get_to(p.SpellLevel);
	j.at("ChanceToLearnSpell").get_to(p.ChanceToLearnSpell);
	j.at("MaxNumberOfSpells").get_to(p.MaxNumberOfSpells);
	j.at("IllusionImmunity").get_to(p.IllusionImmunity);
}

std::vector<IntelligenceAttributes> loadIntelligenceAttributes() {
	std::ifstream i("intelligence.json");

	if (!i.is_open()) {
		game.log("Error: Unable to open intelligence.json. Error " + std::to_string(errno));
		throw std::runtime_error("Error: Unable to open intelligence.json. Error " + std::to_string(errno));
	}

	json j;
	try {
		i >> j;
	}
	catch (const json::parse_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to parse intelligence.json. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}

	try {
		std::vector<IntelligenceAttributes> intelligenceChart = j.get<std::vector<IntelligenceAttributes>>();
		return intelligenceChart;
	}
	catch (const json::type_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to convert JSON to vector<IntelligenceAttributes>. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}
}