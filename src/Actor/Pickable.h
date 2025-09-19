// file: Pickable.h
#ifndef PICKABLE_H
#define PICKABLE_H

#pragma once

#include "../Persistent/Persistent.h"
#include "../Items/Weapons.h" // For WeaponSize and HandRequirement enums

class Actor;
class Creature;
class Item;
class Player; // Forward declaration for smart slot selection

// Forward declaration - EquipmentSlot is defined in Player.h
enum class EquipmentSlot;

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
		// Weapons
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
		// Consumables
		GOLD,
		FOOD,
		CORPSE_FOOD,
		// Potions
		HEALING_POTION,
		MANA_POTION,
		STRENGTH_POTION,
		SPEED_POTION,
		POISON_ANTIDOTE,
		FIRE_RESISTANCE_POTION,
		INVISIBILITY_POTION,
		// Scrolls
		SCROLL_IDENTIFY,
		SCROLL_TELEPORT,
		SCROLL_MAGIC_MAPPING,
		SCROLL_ENCHANTMENT,
		// Equipment
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
	
	// Virtual method for preferred equipment slot - implemented in .cpp to avoid enum forward declaration issues
	virtual EquipmentSlot get_preferred_slot(const Player* player) const;
};

class Dagger : public Weapon
{
public:
	// dagger roll is 1d4
	Dagger() { roll = "D4"; }

	bool is_ranged() const override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::DAGGER; }
	
	// Daggers use smart slot selection - off-hand when main hand occupied by larger weapon
	EquipmentSlot get_preferred_slot(const Player* player) const override;
};


class LongSword : public Weapon
{
public:
	// longsword roll is 1d8
	LongSword() { roll = "D8"; }

	bool is_ranged() const override;
	HandRequirement get_hand_requirement() const override { return HandRequirement::ONE_HANDED; }
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

// Potion classes - consumable items with various effects
class HealingPotion : public Pickable
{
public:
	int heal_amount;
	
	HealingPotion(int amount = 20) : heal_amount(amount) {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::HEALING_POTION; }
};

class ManaPotion : public Pickable
{
public:
	int mana_amount;
	
	ManaPotion(int amount = 15) : mana_amount(amount) {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::MANA_POTION; }
};

class StrengthPotion : public Pickable
{
public:
	int strength_bonus;
	int duration;
	
	StrengthPotion(int bonus = 2, int dur = 100) : strength_bonus(bonus), duration(dur) {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::STRENGTH_POTION; }
};

class SpeedPotion : public Pickable
{
public:
	int speed_bonus;
	int duration;
	
	SpeedPotion(int bonus = 1, int dur = 50) : speed_bonus(bonus), duration(dur) {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SPEED_POTION; }
};

class PoisonAntidote : public Pickable
{
public:
	PoisonAntidote() {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::POISON_ANTIDOTE; }
};

class FireResistancePotion : public Pickable
{
public:
	int resistance_amount;
	int duration;
	
	FireResistancePotion(int resistance = 50, int dur = 200) : resistance_amount(resistance), duration(dur) {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::FIRE_RESISTANCE_POTION; }
};

class InvisibilityPotion : public Pickable
{
public:
	int duration;
	
	InvisibilityPotion(int dur = 30) : duration(dur) {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::INVISIBILITY_POTION; }
};

// Scroll classes - single-use magical effects
class ScrollIdentify : public Pickable
{
public:
	ScrollIdentify() {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SCROLL_IDENTIFY; }
};

class ScrollTeleport : public Pickable
{
public:
	int range;
	
	ScrollTeleport(int teleport_range = 10) : range(teleport_range) {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SCROLL_TELEPORT; }
};

class ScrollMagicMapping : public Pickable
{
public:
	int radius;
	
	ScrollMagicMapping(int map_radius = 25) : radius(map_radius) {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SCROLL_MAGIC_MAPPING; }
};

class ScrollEnchantment : public Pickable
{
public:
	int enhancement_bonus;
	
	ScrollEnchantment(int bonus = 1) : enhancement_bonus(bonus) {}
	
	bool use(Item& owner, Creature& wearer) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::SCROLL_ENCHANTMENT; }
};

// Shield (off-hand defensive item)
class Shield : public Weapon
{
public:
	// shield roll is 1d4 (for shield bash)
	Shield() { roll = "D4"; }

	// Override use method to work like armor
	bool use(Item& owner, Creature& wearer) override;

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