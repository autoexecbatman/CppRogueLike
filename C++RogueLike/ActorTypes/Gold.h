#pragma once
#include "Pickable.h"

class Gold : public Pickable
{
	public:
	int amount{ 0 };

	Gold(int amount);

	bool use(Actor& owner, Actor& wearer) override;

	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};