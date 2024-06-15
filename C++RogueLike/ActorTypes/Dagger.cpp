#include "Dagger.h"
#include "../Actor/Actor.h"
#include "../Items.h"

Dagger::Dagger(int minDmg, int maxDmg) : minDmg(minDmg), maxDmg(maxDmg) {}

bool Dagger::use(Item& owner, Creature& wearer)
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

void Dagger::save(TCODZip& zip)
{
}

void Dagger::load(TCODZip& zip)
{
}