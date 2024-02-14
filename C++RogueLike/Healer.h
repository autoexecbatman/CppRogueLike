#pragma once

#include <libtcod.h>

#include "Actor.h"
#include "Pickable.h"

//==HEALER==
//==
class Healer : public Pickable
{
public:
	int amountToHeal{ 0 }; // how many hp

	Healer(int amountToHeal);

	bool use(Actor& owner, Actor& wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====