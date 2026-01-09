#include "Healer.h"
#include "../Game.h"
#include "../Colors/Colors.h"

//==HEALER==
Healer::Healer(int amountToHeal) : amountToHeal(amountToHeal) {}

bool Healer::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	int amountHealed = wearer.destructible->heal(amountToHeal);

	if (amountHealed > 0)
	{
		game.message(COLOR_WHITE, "You heal ", false);
		game.message(COLOR_RED, std::to_string(amountHealed), false);
		game.message(COLOR_WHITE, " hit points.", true);

		return Pickable::use(owner, wearer, ctx);
	}
	else
	{
		game.message(COLOR_RED, "Health is already maxed out!", true);
	}

	return false;
}

void Healer::load(const json& j)
{
	if (j.contains("amountToHeal") && j["amountToHeal"].is_number())
	{
		amountToHeal = j["amountToHeal"].get<int>();
	}
	else
	{
		throw std::runtime_error("Invalid JSON format for Healer");
	}
}

void Healer::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::HEALER);
	j["amountToHeal"] = amountToHeal;
}

Pickable::PickableType Healer::get_type() const
{
	return PickableType::HEALER;
}