#include "LongSword.h"

LongSword::LongSword(int minDmg, int maxDmg) : minDmg(minDmg), maxDmg(maxDmg) {}

bool LongSword::use(Actor& owner, Actor& wearer)
{
	// equip the weapon
	
	if (!owner.isEquipped)
	{
		wearer.attacker->dmg += 8;
		wearer.equip(owner);
	}
	else
	{
		wearer.attacker->dmg -= 8;
		wearer.unequip(owner);
	}

	// print a star to indicate it is equipped in the inventory screen

	return false;
}

void LongSword::save(TCODZip& zip)
{
}

void LongSword::load(TCODZip& zip)
{
}