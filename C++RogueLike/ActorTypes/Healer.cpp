#include "Healer.h"
#include "../Game.h"
#include "../Colors/Colors.h"

//==HEALER==
Healer::Healer(int amountToHeal) : amountToHeal(amountToHeal) {}

bool Healer::use(Item& owner, Creature& wearer)
{
	int amountHealed = wearer.destructible->heal(amountToHeal);

	if (amountHealed > 0)
	{
		game.message(COLOR_WHITE, "You heal ", false);
		game.message(COLOR_RED, std::to_string(amountHealed), false);
		game.message(COLOR_WHITE, " hit points.", true);

		return Pickable::use(owner, wearer);
	}
	else
	{
		game.message(COLOR_RED, "Health is already maxed out!", true);
	}

	return false;
}

void Healer::load(TCODZip& zip)
{
	amountToHeal = zip.getInt();
}

void Healer::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::HEALER));
	zip.putInt(amountToHeal);
}