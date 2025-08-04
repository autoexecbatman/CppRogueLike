#include "CorpseFood.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include <algorithm>
#include <unordered_map>

// Define nutrition values for different monster types
const std::unordered_map<std::string, int> CORPSE_NUTRITION_VALUES = {
    {"dead goblin", 40},
    {"dead orc", 80},
    {"dead troll", 120},
    {"dead dragon", 200},
    {"dead archer", 70},
    {"dead mage", 60},
    {"dead shopkeeper", 100},
    // Default value for unknown monster types will be assigned in constructor
};

CorpseFood::CorpseFood(int nutrition_value) : nutrition_value(nutrition_value) {}

bool CorpseFood::use(Item& owner, Creature& wearer) {
    // Dynamically calculate nutrition based on corpse type if it wasn't set
    if (nutrition_value <= 0) {
        auto it = CORPSE_NUTRITION_VALUES.find(owner.actorData.name);
        if (it != CORPSE_NUTRITION_VALUES.end()) {
            nutrition_value = it->second;
        }
        else {
            // Default value for unknown corpse types
            nutrition_value = 50;
        }
    }

    // Add a small random variation to make it interesting
    int actualNutrition = nutrition_value + game.d.roll(-10, 10);
    actualNutrition = std::max(10, actualNutrition); // Minimum 10 nutrition

    // Reduce hunger by the nutrition value of the corpse
    game.hunger_system.decrease_hunger(actualNutrition);

    // Generate flavor text based on the corpse type
    std::string flavorText;

    // Determine flavor text based on corpse name
    if (owner.actorData.name.find("goblin") != std::string::npos) {
        flavorText = "It's greasy and gamey.";
    }
    else if (owner.actorData.name.find("orc") != std::string::npos) {
        flavorText = "It's tough and stringy.";
    }
    else if (owner.actorData.name.find("troll") != std::string::npos) {
        flavorText = "It's surprisingly filling, if you can stomach it.";
    }
    else if (owner.actorData.name.find("dragon") != std::string::npos) {
        flavorText = "It tastes exotic and somewhat spicy!";
    }
    else if (owner.actorData.name.find("mage") != std::string::npos) {
        flavorText = "There's a strange aftertaste of magical residue.";
    }
    else if (owner.actorData.name.find("shopkeeper") != std::string::npos) {
        flavorText = "Well-marbled, but you feel guilty...";
    }
    else {
        flavorText = "It tastes... questionable.";
    }

    // Display message
    game.appendMessagePart(WHITE_BLACK_PAIR, "You eat the ");
    game.appendMessagePart(RED_BLACK_PAIR, owner.actorData.name);
    game.appendMessagePart(WHITE_BLACK_PAIR, ". " + flavorText);
    game.finalizeMessage();

    // Corpse is consumed after eating
    return Pickable::use(owner, wearer);
}

void CorpseFood::load(const json& j) {
    if (j.contains("nutrition_value") && j["nutrition_value"].is_number()) {
        nutrition_value = j["nutrition_value"].get<int>();
    }
    else {
        nutrition_value = 50; // Default value
    }
}

void CorpseFood::save(json& j) {
    j["type"] = static_cast<int>(PickableType::CORPSE_FOOD);
    j["nutrition_value"] = nutrition_value;
}