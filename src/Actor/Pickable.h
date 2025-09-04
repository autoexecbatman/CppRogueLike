// file: Pickable.h
#ifndef PICKABLE_H
#define PICKABLE_H

#pragma once

#include "../Persistent/Persistent.h"
#include "../Items/Weapons.h" // For WeaponSize and HandRequirement enums

class Actor;
class Creature;
class Item;

//==PICKABLE==
//==
class Pickable : public Persistent
{
public:
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

	virtual ~Pickable() {};

	virtual bool use(Item& owner, Creature& wearer);
	static std::unique_ptr<Pickable> create(const json& j);
	virtual void save(json& j) = 0;
	virtual void load(const json& j) = 0;
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
	virtual bool is_ranged() const = 0;
	
	// Determines if this weapon can be used both one-handed and two-handed
	// REMOVED: Versatile weapons eliminated for simplicity
	
	// Extensible hand requirement system - replaces dynamic_cast chains
	virtual HandRequirement get_hand_requirement() const { return HandRequirement::ONE_HANDED; }
	bool is_two_handed() const noexcept { return get_hand_requirement() == HandRequirement::TWO_HANDED; }
	
	// AD&D 2e weapon size and dual-wield validation methods
	virtual WeaponSize get_weapon_size() const;
	bool can_be_off_hand(WeaponSize weaponSize) const;
	bool validate_dual_wield(Item* mainHandWeapon, Item* offHandWeapon) const;
};

class Dagger : public Weapon
{
public:
	// dagger roll is 1d4
	Dagger() { roll = "D4"; }

	bool is_ranged() const override;
	WeaponSize get_weapon_size() const override { return WeaponSize::TINY; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::DAGGER; }
};


class LongSword : public Weapon
{
public:
	// longsword roll is 1d8
	LongSword() { roll = "D8"; }

	bool is_ranged() const override;
	HandRequirement get_hand_requirement() const override { return HandRequirement::ONE_HANDED; }
	WeaponSize get_weapon_size() const override { return WeaponSize::MEDIUM; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::LONGSWORD; }
};

class ShortSword : public Weapon
{
public:
	// shortsword roll is 1d6
	ShortSword() { roll = "D6"; }

	bool is_ranged() const override;
	WeaponSize get_weapon_size() const override { return WeaponSize::SMALL; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SHORTSWORD; }
};

class Longbow : public Weapon
{
public:
	// longbow roll is 1d8
	Longbow() { roll = "D8"; }

	bool is_ranged() const override;
	HandRequirement get_hand_requirement() const override { return HandRequirement::TWO_HANDED; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::LONGBOW; }
};

class Staff : public Weapon
{
public:
	// staff roll is 1d6
	Staff() { roll = "D6"; }

	bool is_ranged() const override;
	HandRequirement get_hand_requirement() const override { return HandRequirement::ONE_HANDED; }
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

	bool is_ranged() const override;
	HandRequirement get_hand_requirement() const override { return HandRequirement::TWO_HANDED; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::GREATSWORD; }
};

class BattleAxe : public Weapon
{
public:
	// battle axe roll is 1d8
	BattleAxe() { roll = "D8"; }

	bool is_ranged() const override;
	HandRequirement get_hand_requirement() const override { return HandRequirement::ONE_HANDED; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::BATTLE_AXE; }
};

class GreatAxe : public Weapon
{
public:
	// great axe roll is 1d12
	GreatAxe() { roll = "D12"; }

	bool is_ranged() const override;
	HandRequirement get_hand_requirement() const override { return HandRequirement::TWO_HANDED; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::GREAT_AXE; }
};

class WarHammer : public Weapon
{
public:
	// war hammer roll is 1d8
	WarHammer() { roll = "D8"; }

	bool is_ranged() const override;
	HandRequirement get_hand_requirement() const override { return HandRequirement::ONE_HANDED; }
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

	bool is_ranged() const override;
	HandRequirement get_hand_requirement() const override { return HandRequirement::OFF_HAND_ONLY; }
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SHIELD; }
	
	// Shield provides AC bonus
	virtual int get_ac_bonus() const { return -1; } // +1 AC bonus in AD&D terms
};

#endif // !PICKABLE_H
// end of file: Pickable.h