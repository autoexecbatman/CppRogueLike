#include "Healer.h"

//==HEALER==
Healer::Healer(int amountToHeal) : amountToHeal(amountToHeal) {}

bool Healer::use(Actor& owner, Actor& wearer)
{
	if (wearer.destructible)
	{
		int amountHealed = wearer.destructible->heal(amountToHeal);

		if (amountHealed > 0)
		{
			return Pickable::use(owner, wearer);
		}
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