#pragma once

#include <libtcod.h>

#include "Actor.h"
#include "Pickable.h"

//==CONFUSER==
//==
class Confuser : public Pickable
{
public:
	int nbTurns = 0;
	int range = 0;

	Confuser(int nbTurns, int range) noexcept;

	bool use(Actor& owner, Actor& wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====