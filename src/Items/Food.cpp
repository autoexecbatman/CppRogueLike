#include "Food.h"
#include "../Game.h"
#include "../Colors/Colors.h"

Food::Food(int nutrition_value) : nutrition_value(nutrition_value) {}

bool Food::use(Item& owner, Creature& wearer) {
    // Reduce hunger by the nutrition value of the food
    game.hunger_system.decrease_hunger(nutrition_value);

    // Display message
    game.appendMessagePart(WHITE_BLACK_PAIR, "You eat the ");
    game.appendMessagePart(YELLOW_BLACK_PAIR, owner.actorData.name);
    game.appendMessagePart(WHITE_BLACK_PAIR, ".");
    game.finalizeMessage();

    // Food is consumed after being eaten
    return Pickable::use(owner, wearer);
}

void Food::load(const json& j) {
    nutrition_value = j["nutrition_value"].get<int>();
}

void Food::save(json& j) {
    j["type"] = static_cast<int>(PickableType::FOOD);
    j["nutrition_value"] = nutrition_value;
}

// Implementations for different food types
Ration::Ration(Vector2D position) : Item(position, ActorData{ '%', "ration", WHITE_GREEN_PAIR }) {
    pickable = std::make_unique<Food>(300);  // High nutritional value
    value = 10;  // Base gold value
}

Fruit::Fruit(Vector2D position) : Item(position, ActorData{ '%', "fruit", GREEN_BLACK_PAIR }) {
    pickable = std::make_unique<Food>(100);  // Medium nutritional value
    value = 3;  // Base gold value
}

Bread::Bread(Vector2D position) : Item(position, ActorData{ '%', "bread", RED_YELLOW_PAIR }) {
    pickable = std::make_unique<Food>(200);  // Medium-high nutritional value
    value = 5;  // Base gold value
}

Meat::Meat(Vector2D position) : Item(position, ActorData{ '%', "meat", RED_BLACK_PAIR }) {
    pickable = std::make_unique<Food>(250);  // High nutritional value
    value = 8;  // Base gold value
}