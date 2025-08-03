#include "Gold.h"
#include "../Actor/Actor.h"
#include "../Game.h"

Gold::Gold(int amount) : amount(amount)
{
}

bool Gold::use(Item& owner, Creature& wearer)
{
    // Add gold amount to player's currency
    wearer.gold += amount;

    // Display a message
    game.appendMessagePart(YELLOW_BLACK_PAIR, "You gained ");
    game.appendMessagePart(YELLOW_BLACK_PAIR, std::to_string(amount));
    game.appendMessagePart(YELLOW_BLACK_PAIR, " gold.");
    game.finalizeMessage();

    // Return true to consume the item
    return Pickable::use(owner, wearer);
}

void Gold::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::GOLD); // Save the type
	j["amount"] = amount; // Save the amount
}

void Gold::load(const json& j)
{
	if (j.contains("amount") && j["amount"].is_number_integer()) {
		amount = j["amount"].get<int>(); // Load the amount
	}
	else {
		throw std::runtime_error("Invalid JSON format for Gold");
	}
}
