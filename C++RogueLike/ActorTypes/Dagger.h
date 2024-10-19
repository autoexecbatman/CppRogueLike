#ifndef DAGGER_H
#define DAGGER_H

#pragma once

#include "../Actor/Pickable.h"

class Dagger : public Pickable
{
public:
	// dagger roll is 1d4
	std::string_view roll{ "D4" };

	bool use(Item& owner, Creature& wearer) override;

	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};

#endif // !DAGGER_H