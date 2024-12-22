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
		HEALER,
		LIGHTNING_BOLT,
		CONFUSER,
		FIREBALL,
		LONGSWORD,
		DAGGER,
		SHORTSWORD,
		LONGBOW,
		STAFF
	};
};
//====

class Dagger : public Pickable
{
public:
	// dagger roll is 1d4
	std::string_view roll{ "D4" };

	bool use(Item& owner, Creature& wearer) override;

	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};


class LongSword : public Pickable
{
public:
	// longsword roll is 1d8
	std::string_view roll{ "D8" };

	bool use(Item& owner, Creature& wearer) override;

	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};

class ShortSword : public Pickable
{
public:
	// shortsword roll is 1d6
	std::string_view roll{ "D6" };
	bool use(Item& owner, Creature& wearer) override;
	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};

class Longbow : public Pickable
{
public:
	// longbow roll is 1d8
	std::string_view roll{ "D8" };
	bool use(Item& owner, Creature& wearer) override;
	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};

class Staff : public Pickable
{
public:
	// staff roll is 1d6
	std::string_view roll{ "D6" };
	bool use(Item& owner, Creature& wearer) override;
	void save(TCODZip& zip) override;
	void load(TCODZip& zip) override;
};

#endif // !PICKABLE_H
// end of file: Pickable.h
