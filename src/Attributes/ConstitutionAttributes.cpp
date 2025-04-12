#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "ConstitutionAttributes.h"
#include "../Game.h"

using json = nlohmann::json;

void from_json(const json& j, ConstitutionAttributes& c)
{
    j.at("Con").get_to(c.Con);
    j.at("HPAdj").get_to(c.HPAdj);
    j.at("SystemShock").get_to(c.SystemShock);
    j.at("ResurrectionSurvival").get_to(c.ResurrectionSurvival);
    j.at("PoisonSave").get_to(c.PoisonSave);
    j.at("Regeneration").get_to(c.Regeneration);
}

std::vector<ConstitutionAttributes> loadConstitutionAttributes()
{
    std::ifstream i("constitution.json");

    if (!i.is_open()) {
        game.log("Error: Unable to open constitution.json. Error " + std::to_string(errno));
        throw std::runtime_error("Error: Unable to open constitution.json. Error " + std::to_string(errno));
    }

    json j;
    try {
        i >> j;
    }
    catch (const json::parse_error& e) {
        std::ostringstream oss;
        oss << "Error: Unable to parse constitution.json. Exception details: " << e.what();
        throw std::runtime_error(oss.str());
    }

    try {
        std::vector<ConstitutionAttributes> constitutionChart = j.get<std::vector<ConstitutionAttributes>>();
        return constitutionChart;
    }
    catch (const json::type_error& e) {
        std::ostringstream oss;
        oss << "Error: Unable to convert JSON to vector<ConstitutionAttributes>. Exception details: " << e.what();
        throw std::runtime_error(oss.str());
    }
}