#pragma once
#include "../Actor/Pickable.h"

class Gold : public Pickable
{
	public:
	int amount{ 0 };

	Gold(int amount);

	bool use(Item& owner, Creature& wearer) override;

	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};