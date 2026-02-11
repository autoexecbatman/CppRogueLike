#pragma once

#include "../Persistent/Persistent.h"
#include "../Items/Weapons.h" // For WeaponSize and HandRequirement enums
#include "../Systems/TargetMode.h"
#include "../Systems/BuffType.h"

class Actor;
class Creature;
class Item;
struct GameContext;

#include "EquipmentSlot.h"

//==PICKABLE==
class Pickable : public Persistent
{
public:
	// Behavioral category tags - NOT item identity (ItemId is the identity)
	enum class PickableType
	{
		// Scroll / ranged effects (complex targeting behavior)
		TARGETED_SCROLL,
		TELEPORTER,

		// Weapons
		WEAPON,  // all weapons - config stored as data in Weapon
		SHIELD,  // special: off-hand only, toggle_shield dispatch

		// Consumables (potions + simple scrolls)
		CONSUMABLE,

		// Treasure & food
		GOLD,
		FOOD,
		CORPSE_FOOD,

		// Equipment
		ARMOR,
		MAGICAL_HELM,
		MAGICAL_RING,
		JEWELRY_AMULET,
		GAUNTLETS,
		GIRDLE,

		// Special
		QUEST_ITEM,
	};

	virtual ~Pickable() = default;

	// Polymorphic base class - disable copy, enable move
	Pickable() = default;
	Pickable(const Pickable&) = delete;
	Pickable& operator=(const Pickable&) = delete;
	Pickable(Pickable&&) = default;
	Pickable& operator=(Pickable&&) = default;

	virtual bool use(Item& owner, Creature& wearer, GameContext& ctx);
	static std::unique_ptr<Pickable> create(const json& j);
	virtual void save(json& j) = 0;
	virtual void load(const json& j) = 0;
	virtual PickableType get_type() const = 0;

	// Virtual AC bonus - allows polymorphic AC calculation without dynamic_cast
	virtual int get_ac_bonus() const noexcept { return 0; }
};
//====

// Effect type for Consumable - what happens when the item is used
enum class ConsumableEffect : int
{
	NONE = 0,  // Show message, consume item
	HEAL,      // Heal HP, consume item
	ADD_BUFF,  // Add timed buff via BuffSystem, consume item
	FAIL,      // Show message, do NOT consume (feature not yet implemented)
};

// Single class for all consumable items (potions, simple scrolls)
// All behavior is data-driven: effect type + parameters set at construction
class Consumable : public Pickable
{
public:
	ConsumableEffect effect;
	int amount;           // heal_amount (HEAL) or buff value (ADD_BUFF)
	int duration;         // buff duration (ADD_BUFF)
	BuffType buff_type;   // buff effect type (ADD_BUFF)
	bool is_set_effect;   // true = SET stat to value (potions), false = ADD value

	Consumable(ConsumableEffect e, int amt, int dur, BuffType bt, bool set_effect = false)
		: effect(e), amount(amt), duration(dur), buff_type(bt), is_set_effect(set_effect) {}

	bool use(Item& owner, Creature& wearer, GameContext& ctx) override;
	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::CONSUMABLE; }
};

// Data-driven weapon class - all config stored as member data
class Weapon : public Pickable
{
protected:
	bool ranged;
	HandRequirement hand_requirement;
	WeaponSize weapon_size;

public:
	Weapon(bool is_ranged, HandRequirement hands, WeaponSize size)
		: ranged(is_ranged), hand_requirement(hands), weapon_size(size) {}

	// Common weapon equip/unequip logic
	bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

	// Data accessors
	bool is_ranged() const noexcept { return ranged; }
	HandRequirement get_hand_requirement() const noexcept { return hand_requirement; }
	bool is_two_handed() const noexcept { return hand_requirement == HandRequirement::TWO_HANDED; }
	WeaponSize get_weapon_size() const noexcept { return weapon_size; }

	// AD&D 2e dual-wield validation
	bool can_be_off_hand(WeaponSize weaponSize) const;
	bool validate_dual_wield(Item* mainHandWeapon, Item* offHandWeapon) const;

	// Virtual for slot selection (e.g. can_be_off_hand weapons)
	virtual EquipmentSlot get_preferred_slot(const Creature* creature) const;

	void save(json& j) override;
	void load(const json& j) override;
	PickableType get_type() const override { return PickableType::WEAPON; }
};

class TargetedScroll : public Pickable
{
public:
    TargetMode target_mode{TargetMode::AUTO_NEAREST};
    ScrollAnimation scroll_animation{ScrollAnimation::NONE};
    int range{0};
    int damage{0};
    int confuse_turns{0};

    TargetedScroll(TargetMode mode, ScrollAnimation anim, int rng, int dmg, int confuse)
        : target_mode(mode), scroll_animation(anim), range(rng), damage(dmg), confuse_turns(confuse) {}

    bool use(Item& owner, Creature& wearer, GameContext& ctx) override;
    void save(json& j) override;
    void load(const json& j) override;
    PickableType get_type() const override { return PickableType::TARGETED_SCROLL; }
};

// Shield (off-hand defensive item)
class Shield : public Weapon
{
public:
	Shield() : Weapon(false, HandRequirement::OFF_HAND_ONLY, WeaponSize::MEDIUM) {}

	// Override use method - shield equips to left hand via toggle_shield
	bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

	// Shield provides AC bonus
	int get_ac_bonus() const noexcept override { return -1; } // +1 AC bonus in AD&D terms
};
