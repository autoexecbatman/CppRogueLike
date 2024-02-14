#ifndef LONGSWORD_H
#define LONGSWORD_H

#pragma once

#include "Pickable.h"
#include "Actor.h"
#include "RandomDice.h"

class LongSword : public Pickable
{
public:
	int damage{ 0 };
	LongSword(int dmg);
	bool use(Actor& owner, Actor& wearer) override;
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;
};

#endif // !LONGSWORD_H