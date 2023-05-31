#ifndef LIGHTNING_BOLT_H
#define LIGHTNING_BOLT_H

#pragma once

#include <libtcod.h>

#include "Actor.h"
#include "Pickable.h"

//==LIGHTNING_BOLT==
//==
class LightningBolt : public Pickable
{
public:
	int maxRange = 0;
	int damage = 0;

	LightningBolt(int range, int damage);

	bool use(Actor& owner, Actor& wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====
#endif // !LIGHTNING_BOLT_H
