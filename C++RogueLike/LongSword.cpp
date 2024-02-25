#include "LongSword.h"

LongSword::LongSword(int minDmg, int maxDmg) : minDmg(minDmg), maxDmg(maxDmg) {}

bool LongSword::use(Actor& owner, Actor& wearer)
{
	// equip the weapon
	if (!owner.isEquipped)
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

void LongSword::save(TCODZip& zip)
{
}

void LongSword::load(TCODZip& zip)
{
}