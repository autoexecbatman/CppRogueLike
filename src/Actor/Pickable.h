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
	static std::unique_ptr<Pickable> create(const json& j);
	virtual void save(json& j) = 0;
	virtual void load(const json& j) = 0;
	
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
		STAFF,
		GOLD,
		FOOD
	};

	virtual PickableType get_type() const = 0;
};
//====

class Dagger : public Pickable
{
public:
	// dagger roll is 1d4
	std::string roll{ "D4" };

	bool use(Item& owner, Creature& wearer) override;

	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::DAGGER; }
};


class LongSword : public Pickable
{
public:
	// longsword roll is 1d8
	std::string roll{ "D8" };

	bool use(Item& owner, Creature& wearer) override;

	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::LONGSWORD; }
};

class ShortSword : public Pickable
{
public:
	// shortsword roll is 1d6
	std::string roll{ "D6" };
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SHORTSWORD; }
};

class Longbow : public Pickable
{
public:
	// longbow roll is 1d8
	std::string roll{ "D8" };
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::LONGBOW; }
};

class Staff : public Pickable
{
public:
	// staff roll is 1d6
	std::string roll{ "D6" };
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::STAFF; }
};

#endif // !PICKABLE_H
// end of file: Pickable.h
