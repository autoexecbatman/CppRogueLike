// file: Pickable.h
#pragma once

#include <variant>

#include "../Items/MagicalItemEffects.h"
#include "../Items/Weapons.h"
#include "../Persistent/Persistent.h"
#include "../Systems/BuffType.h"
#include "../Systems/TargetMode.h"
#include "EquipmentSlot.h"

class Item;
class Creature;
struct GameContext;

// ========== Serialization discriminator ==========

enum class PickableType
{
	TARGETED_SCROLL,
	TELEPORTER,
	WEAPON,
	SHIELD,
	CONSUMABLE,
	GOLD_COIN,
	FOOD,
	CORPSE_FOOD,
	ARMOR,
	MAGICAL_HELM,
	MAGICAL_RING,
	JEWELRY_AMULET,
	GAUNTLETS,
	GIRDLE,
	QUEST_ITEM,
};

// Effect type for Consumable
enum class ConsumableEffect
{
	NONE,
	HEAL,
	ADD_BUFF,
	FAIL,
};

// ========== Plain data structs (no base class, no virtuals) ==========

struct Consumable
{
	ConsumableEffect effect{ ConsumableEffect::NONE };
	int amount{ 0 };
	int duration{ 0 };
	BuffType buffType{ BuffType::NONE };
	bool isSetEffect{ false };
};

struct Weapon
{
	bool ranged{ false };
	HandRequirement handRequirement{ HandRequirement::ONE_HANDED };
	WeaponSize weaponSize{ WeaponSize::MEDIUM };

	bool is_ranged() const noexcept { return ranged; }
	bool is_two_handed() const noexcept { return handRequirement == HandRequirement::TWO_HANDED; }
	WeaponSize get_weapon_size() const noexcept { return weaponSize; }
	HandRequirement get_hand_requirement() const noexcept { return handRequirement; }
	bool can_be_off_hand() const noexcept { return weaponSize <= WeaponSize::SMALL; }
	bool validate_dual_wield(const Item* main_hand, const Item* off_hand) const;
	EquipmentSlot get_preferred_slot(const Creature* creature) const;
};

struct Shield
{
};

struct TargetedScroll
{
	TargetMode targetMode{ TargetMode::AUTO_NEAREST };
	ScrollAnimation scrollAnimation{ ScrollAnimation::NONE };
	int range{ 0 };
	int damage{ 0 };
	int confuseTurns{ 0 };
	BuffType buffType{ BuffType::NONE };
	int buffDuration{ 0 };
};

struct Teleporter
{
};

struct Gold
{
	int amount{ 0 };
};

struct Food
{
	int nutritionValue{ 0 };
};

struct CorpseFood
{
	int nutritionValue{ 0 };
};

struct Armor
{
	int armorClass{ 0 };
};

struct MagicalHelm
{
	MagicalEffect effect{ MagicalEffect::NONE };
	int bonus{ 0 };
};

struct MagicalRing
{
	MagicalEffect effect{ MagicalEffect::NONE };
	int bonus{ 0 };
};

// Original stats before a SET operation (stored on equip, restored on unequip)
struct OriginalStats
{
	int str{ 0 };
	int dex{ 0 };
	int con{ 0 };
	int intel{ 0 };
	int wis{ 0 };
	int cha{ 0 };
};

struct JewelryAmulet
{
	int strBonus{ 0 };
	int dexBonus{ 0 };
	int conBonus{ 0 };
	int intBonus{ 0 };
	int wisBonus{ 0 };
	int chaBonus{ 0 };
	MagicalEffect effect{ MagicalEffect::NONE };
	int bonus{ 0 };
	bool isSetMode{ false };
	OriginalStats originalStats{};
};

struct Gauntlets
{
	int strBonus{ 0 };
	int dexBonus{ 0 };
	int conBonus{ 0 };
	int intBonus{ 0 };
	int wisBonus{ 0 };
	int chaBonus{ 0 };
	MagicalEffect effect{ MagicalEffect::NONE };
	int bonus{ 0 };
	bool isSetMode{ false };
	OriginalStats originalStats{};
};

struct Girdle
{
	int strBonus{ 0 };
	int dexBonus{ 0 };
	int conBonus{ 0 };
	int intBonus{ 0 };
	int wisBonus{ 0 };
	int chaBonus{ 0 };
	MagicalEffect effect{ MagicalEffect::NONE };
	int bonus{ 0 };
	bool isSetMode{ false };
	OriginalStats originalStats{};
};

struct Amulet
{
};

// ========== The variant ==========

using ItemBehavior = std::variant<
	Consumable,
	Weapon,
	Shield,
	TargetedScroll,
	Teleporter,
	Gold,
	Food,
	CorpseFood,
	Armor,
	MagicalHelm,
	MagicalRing,
	JewelryAmulet,
	Gauntlets,
	Girdle,
	Amulet>;

// ========== use() overloads - one per behavior type ==========

bool use(Consumable& c, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Weapon& w, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Shield& s, Item& owner, Creature& wearer, GameContext& ctx);
bool use(TargetedScroll& targetScroll, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Teleporter& t, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Gold& g, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Food& f, Item& owner, Creature& wearer, GameContext& ctx);
bool use(CorpseFood& cf, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Armor& a, Item& owner, Creature& wearer, GameContext& ctx);
bool use(MagicalHelm& mh, Item& owner, Creature& wearer, GameContext& ctx);
bool use(MagicalRing& mr, Item& owner, Creature& wearer, GameContext& ctx);
bool use(JewelryAmulet& ja, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Gauntlets& g, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Girdle& g, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Amulet& a, Item& owner, Creature& wearer, GameContext& ctx);

// ========== get_ac_bonus() overloads ==========

int get_ac_bonus(const Consumable&) noexcept;
int get_ac_bonus(const Weapon&) noexcept;
int get_ac_bonus(const Shield&) noexcept;
int get_ac_bonus(const TargetedScroll&) noexcept;
int get_ac_bonus(const Teleporter&) noexcept;
int get_ac_bonus(const Gold&) noexcept;
int get_ac_bonus(const Food&) noexcept;
int get_ac_bonus(const CorpseFood&) noexcept;
int get_ac_bonus(const Armor& a) noexcept;
int get_ac_bonus(const MagicalHelm& mh) noexcept;
int get_ac_bonus(const MagicalRing& mr) noexcept;
int get_ac_bonus(const JewelryAmulet&) noexcept;
int get_ac_bonus(const Gauntlets&) noexcept;
int get_ac_bonus(const Girdle&) noexcept;
int get_ac_bonus(const Amulet&) noexcept;

// ========== Variant-level dispatchers ==========

bool use_item(ItemBehavior& behavior, Item& owner, Creature& wearer, GameContext& ctx);
int get_item_ac_bonus(const ItemBehavior& behavior) noexcept;

// Serialization
void save_behavior(const ItemBehavior& behavior, json& j);
ItemBehavior load_behavior(const json& j);
