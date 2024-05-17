#include "Dagger.h"

Dagger::Dagger(int minDmg, int maxDmg) : minDmg(minDmg), maxDmg(maxDmg) {}

bool Dagger::use(Actor& owner, Actor& wearer)
{
	// equip the weapon

	if (!owner.flags.isEquipped)
	{
		wearer.attacker->dmg += maxDmg;
		wearer.equip(owner);
	}
	else
	{
		wearer.attacker->dmg -= maxDmg;
		wearer.unequip(owner);
	}

	return false;
}

void Dagger::save(TCODZip& zip)
{
}

void Dagger::load(TCODZip& zip)
{
}