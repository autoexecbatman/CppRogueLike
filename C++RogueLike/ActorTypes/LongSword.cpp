#include "LongSword.h"

bool LongSword::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	else
	{
		wearer.attacker->roll = this->roll;
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