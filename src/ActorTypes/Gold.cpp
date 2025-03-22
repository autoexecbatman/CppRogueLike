#include "Gold.h"

Gold::Gold(int amount)
{
}

bool Gold::use(Item& owner, Creature& wearer)
{
	return false;
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
