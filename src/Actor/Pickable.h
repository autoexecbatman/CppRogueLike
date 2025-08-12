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
  GREATSWORD,
  BATTLE_AXE,
  GREAT_AXE,
  WAR_HAMMER,
  SHIELD,
  GOLD,
  FOOD,
 CORPSE_FOOD,
AMULET,
LEATHER_ARMOR,
CHAIN_MAIL,
PLATE_MAIL
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
	
	// Determines if this weapon can be used both one-handed and two-handed
	virtual bool isVersatile() const { return false; }
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
	bool isVersatile() const override { return true; }
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
	bool isVersatile() const override { return true; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::STAFF; }
};

// Two-handed weapons
class Greatsword : public Weapon
{
public:
	// greatsword roll is 1d12 (equivalent to 2d6 average)
	Greatsword() { roll = "D12"; }

	bool isRanged() const override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::GREATSWORD; }
};

class BattleAxe : public Weapon
{
public:
	// battle axe roll is 1d8 (1d10 when two-handed)
	BattleAxe() { roll = "D8"; }

	bool isRanged() const override;
	bool isVersatile() const override { return true; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::BATTLE_AXE; }
};

class GreatAxe : public Weapon
{
public:
	// great axe roll is 1d12
	GreatAxe() { roll = "D12"; }

	bool isRanged() const override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::GREAT_AXE; }
};

class WarHammer : public Weapon
{
public:
	// war hammer roll is 1d8 (1d10 when two-handed)
	WarHammer() { roll = "D8"; }

	bool isRanged() const override;
	bool isVersatile() const override { return true; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::WAR_HAMMER; }
};

// Shield (off-hand defensive item)
class Shield : public Weapon
{
public:
	// shield roll is 1d4 (for shield bash)
	Shield() { roll = "D4"; }

	bool isRanged() const override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SHIELD; }
};

#endif // !PICKABLE_H
// end of file: Pickable.h