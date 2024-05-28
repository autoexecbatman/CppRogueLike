#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"

//==HEALER==
//==
class Healer : public Pickable
{
public:
	int amountToHeal{ 0 }; // how many hp

	Healer(int amountToHeal);

	bool use(Item& owner, Creature& wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====