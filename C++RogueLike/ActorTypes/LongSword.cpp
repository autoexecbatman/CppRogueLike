#include "LongSword.h"

LongSword::LongSword(int minDmg, int maxDmg) : minDmg(minDmg), maxDmg(maxDmg) {}

bool LongSword::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
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