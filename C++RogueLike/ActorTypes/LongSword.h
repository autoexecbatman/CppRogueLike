#ifndef LONGSWORD_H
#define LONGSWORD_H

#pragma once

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../Items.h"

class LongSword : public Pickable
{
public:
	int minDmg{ 1 };
	int maxDmg{ 8 };
	// longsword roll is 1d8
	LongSword(int minDmg, int maxDmg);
	
	bool use(Item& owner, Creature& wearer) override;

	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};

#endif // !LONGSWORD_H