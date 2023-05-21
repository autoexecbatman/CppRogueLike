#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "StrengthAttributes.h"

using json = nlohmann::json;

void from_json(const json& j, StrengthAttributes& p) {
	j.at("Hit").get_to(p.hitProb);
	j.at("Dmg").get_to(p.dmgAdj);
	j.at("Wgt").get_to(p.wgtAllow);
	j.at("MaxPress").get_to(p.maxPress);
	j.at("OpenDoors").get_to(p.openDoors);
	j.at("BB_LG").get_to(p.BB_LG);
	j.at("Notes").get_to(p.notes);
}

std::vector<StrengthAttributes> loadStrengthAttributes() {
	std::ifstream i("strength.json");
	json j;
	i >> j;

	std::vector<StrengthAttributes> strengthChart = j.get<std::vector<StrengthAttributes>>();
	return strengthChart;
}

void print_chart()
{
	auto strengthChart = loadStrengthAttributes();
	for (const auto& strength : strengthChart)
	{
		std::cout << "----" << std::endl;
		std::cout << "strength.hitProb: " << strength.hitProb << std::endl;
		std::cout << "strength.dmgAdj: " << strength.dmgAdj << std::endl;
		std::cout << "strength.wgtAllow: " << strength.wgtAllow << std::endl;
		std::cout << "strength.maxPress: " << strength.maxPress << std::endl;
		std::cout << "strength.openDoors: " << strength.openDoors << std::endl;
		std::cout << "strength.BB_LG: " << strength.BB_LG << std::endl;
		std::cout << "strength.notes: " << strength.notes << std::endl;
		std::cout << "----" << std::endl;
	}
}
