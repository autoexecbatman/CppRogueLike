#ifndef DAGGER_H
#define DAGGER_H

#pragma once

#include "../Actor/Pickable.h"

class Dagger : public Pickable
{
public:
	int minDmg{ 1 };
	int maxDmg{ 4 };
	// dagger roll is 1d4
	Dagger(int minDmg, int maxDmg);

	bool use(Item& owner, Creature& wearer) override;

	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};

#endif // !DAGGER_H