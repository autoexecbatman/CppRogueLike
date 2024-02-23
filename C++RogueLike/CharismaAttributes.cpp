#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "Game.h"
#include "CharismaAttributes.h"

using json = nlohmann::json;

void from_json(const json& j, CharismaAttributes& c) {
	j.at("Cha").get_to(c.Cha);
	j.at("MaxHencmen").get_to(c.MaxHencmen);
	j.at("Loyalty").get_to(c.Loyalty);
	j.at("ReactionAdj").get_to(c.ReactionAdj);
}

std::vector<CharismaAttributes> loadCharismaAttributes()
{
	std::ifstream i("charisma.json");

	if (!i.is_open()) {
		game.log("Error: Unable to open charisma.json. Error " + std::to_string(errno));
		throw std::runtime_error("Error: Unable to open charisma.json. Error " + std::to_string(errno));
	}

	json j;
	try {
		i >> j;
	}
	catch (const json::parse_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to parse charisma.json. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}

	try {
		std::vector<CharismaAttributes> charismaChart = j.get<std::vector<CharismaAttributes>>();
		return charismaChart;
	}
	catch (const json::type_error& e) {
		std::ostringstream oss;
		oss << "Error: Unable to convert JSON to vector<CharismaAttributes>. Exception details: " << e.what();
		throw std::runtime_error(oss.str());
	}
}

void CharismaAttributes::print_chart()
{
	auto charismaChart = loadCharismaAttributes();
	for (const auto& charisma : charismaChart)
	{
		std::cout << "----" << std::endl;
		std::cout << "charisma.Cha: " << charisma.Cha << std::endl;
		std::cout << "charisma.MaxHencmen: " << charisma.MaxHencmen << std::endl;
		std::cout << "charisma.Loyalty: " << charisma.Loyalty << std::endl;
		std::cout << "charisma.ReactionAdj: " << charisma.ReactionAdj << std::endl;
		std::cout << "----" << std::endl;
	}
}