#pragma once

#include "Pickable.h"

class TCODZip;
class Creature;
class Item;

//==CONFUSER==
//==
class Confuser : public Pickable
{
public:
	int nbTurns = 0;
	int range = 0;

	Confuser(int nbTurns, int range) noexcept;

	bool use(Item& owner, Creature& wearer);

	void load(TCODZip& zip);
	void save(TCODZip& zip);
};
//====