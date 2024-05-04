#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "LightningBolt.h"

//==FIREBALL==
//==
class Fireball : public LightningBolt
{
public:
	Fireball(int range, int damage);

	bool use(Actor& owner, Actor& wearer);

	void animation(int x, int y, int maxRange);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====