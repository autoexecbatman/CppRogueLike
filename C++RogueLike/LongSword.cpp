#include "LongSword.h"

LongSword::LongSword(int dmg) : damage(dmg)
{
}

bool LongSword::use(Actor& owner, Actor& wearer)
{


	return Pickable::use(owner,wearer);
}

void LongSword::load(TCODZip& zip)
{
}

void LongSword::save(TCODZip& zip)
{
}
