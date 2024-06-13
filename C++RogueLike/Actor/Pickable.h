// file: Pickable.h
#ifndef PICKABLE_H
#define PICKABLE_H

#pragma once

#include "../Persistent/Persistent.h"
class Actor;
class Creature;
class Item;

//==PICKABLE==
//==
class Pickable : public Persistent
{
public:
	virtual ~Pickable() {};

	virtual bool use(Item& owner, Creature& wearer);
	/*static Pickable* create(TCODZip& zip);*/
	static std::unique_ptr<Pickable> create(TCODZip& zip);
	
protected:
	enum class PickableType : int
	{
		HEALER, LIGHTNING_BOLT, CONFUSER, FIREBALL, LONGSWORD, DAGGER
	};
};
//====

#endif // !PICKABLE_H
// end of file: Pickable.h
