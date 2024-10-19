#ifndef LONGSWORD_H
#define LONGSWORD_H

#pragma once

#include "../Actor/Actor.h"
#include "../Actor/Pickable.h"
#include "../Items.h"

class LongSword : public Pickable
{
public:
	// longsword roll is 1d8
	std::string_view roll{ "D8" };
	
	bool use(Item& owner, Creature& wearer) override;

	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};

#endif // !LONGSWORD_H