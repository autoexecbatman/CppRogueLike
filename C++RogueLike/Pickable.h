// file: Pickable.h
#ifndef PICKABLE_H
#define PICKABLE_H

#pragma once

#include "Persistent.h"
class Actor;

//==PICKABLE==
//==
class Pickable : public Persistent
{
public:
	virtual ~Pickable() {};

	bool pick(std::unique_ptr<Actor> owner, const Actor& wearer);
	void drop(std::unique_ptr<Actor> owner, Actor& wearer);

	virtual bool use(Actor& owner, Actor& wearer);
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
