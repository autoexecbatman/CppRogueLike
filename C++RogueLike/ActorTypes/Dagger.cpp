#include "Dagger.h"
#include "../Actor/Actor.h"
#include "../Items.h"

bool Dagger::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	// unequip the weapon
	else
	{
		wearer.attacker->roll = this->roll;
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