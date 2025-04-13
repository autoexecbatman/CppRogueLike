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
		FOOD,
		CORPSE_FOOD,
		AMULET,
		LEATHER_ARMOR
	};

	virtual PickableType get_type() const = 0;
};
//====

// Base weapon class to reduce code duplication
class Weapon : public Pickable
{
public:
	std::string roll;

	// Common weapon equip/unequip logic
	bool use(Item& owner, Creature& wearer) override;

	// Determines if this weapon is a ranged weapon
	virtual bool isRanged() const = 0;
};

class Dagger : public Weapon
{
public:
	// dagger roll is 1d4
	Dagger() { roll = "D4"; }

	bool isRanged() const override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::DAGGER; }
};


class LongSword : public Weapon
{
public:
	// longsword roll is 1d8
	LongSword() { roll = "D8"; }

	bool isRanged() const override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::LONGSWORD; }
};

class ShortSword : public Weapon
{
public:
	// shortsword roll is 1d6
	ShortSword() { roll = "D6"; }

	bool isRanged() const override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SHORTSWORD; }
};

class Longbow : public Weapon
{
public:
	// longbow roll is 1d8
	Longbow() { roll = "D8"; }

	bool isRanged() const override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::LONGBOW; }
};

class Staff : public Weapon
{
public:
	// staff roll is 1d6
	Staff() { roll = "D6"; }

	bool isRanged() const override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::STAFF; }
};

#endif // !PICKABLE_H
// end of file: Pickable.h