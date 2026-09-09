#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

#include <variant>

#include "../Items/MagicalItemEffects.h"
#include "../Items/Weapons.h"
#include "../Persistent/Persistent.h"
#include "../Systems/BuffType.h"
#include "../Systems/TargetMode.h"
#include "EquipmentSlot.h"

class Item;
class Creature;
class Player;
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
	IDENTIFY_SCROLL,
	DUNGEON_KEY,
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
	bool validate_dual_wield(const Item* mainHand, const Item* offHand) const;
	EquipmentSlot get_preferred_slot(const Player* player) const;
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

struct IdentifyScroll
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

// Used on a locked door via bump — consumed automatically by PlayerController.
// When selected from inventory, shows a hint message.
struct DungeonKey
{
};

// ========== The variant ==========

using ItemBehavior = std::variant<
	Consumable,
	Weapon,
	Shield,
	TargetedScroll,
	Teleporter,
	IdentifyScroll,
	Gold,
	Food,
	CorpseFood,
	Armor,
	MagicalHelm,
	MagicalRing,
	JewelryAmulet,
	Gauntlets,
	Girdle,
	Amulet,
	DungeonKey>;

// ========== use() overloads - one per behavior type ==========

bool use(Consumable& c, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Weapon& w, Item& owner, Player& wearer, GameContext& ctx);
bool use(Shield& s, Item& owner, Player& wearer, GameContext& ctx);
bool use(TargetedScroll& targetScroll, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Teleporter& t, Item& owner, Creature& wearer, GameContext& ctx);
bool use(IdentifyScroll& is, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Gold& g, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Food& f, Item& owner, Creature& wearer, GameContext& ctx);
bool use(CorpseFood& cf, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Armor& armor, Item& item, Player& wearer, GameContext& ctx);
bool use(MagicalHelm& mh, Item& owner, Player& wearer, GameContext& ctx);
bool use(MagicalRing& mr, Item& owner, Player& wearer, GameContext& ctx);
bool use(JewelryAmulet& ja, Item& owner, Player& wearer, GameContext& ctx);
bool use(Gauntlets& g, Item& owner, Player& wearer, GameContext& ctx);
bool use(Girdle& g, Item& owner, Player& wearer, GameContext& ctx);
bool use(Amulet& a, Item& owner, Creature& wearer, GameContext& ctx);
bool use(DungeonKey& key, Item& owner, Creature& wearer, GameContext& ctx);

// ========== Variant-level dispatchers ==========

bool use_item(ItemBehavior& behavior, Item& owner, Player& wearer, GameContext& ctx);
int get_item_ac_bonus(const ItemBehavior& behavior) noexcept;

// Serialization
void save_behavior(const ItemBehavior& behavior, json& j);
ItemBehavior load_behavior(const json& j);

inline std::string_view encode_pickable_type(PickableType t)
{
	switch (t)
	{

	case PickableType::TARGETED_SCROLL:
	{
		return "targeted_scroll";
	}

	case PickableType::TELEPORTER:
	{
		return "teleporter";
	}

	case PickableType::IDENTIFY_SCROLL:
	{
		return "identify_scroll";
	}

	case PickableType::WEAPON:
	{
		return "weapon";
	}

	case PickableType::SHIELD:
	{
		return "shield";
	}

	case PickableType::CONSUMABLE:
	{
		return "consumable";
	}

	case PickableType::GOLD_COIN:
	{
		return "gold_coin";
	}

	case PickableType::FOOD:
	{
		return "food";
	}

	case PickableType::CORPSE_FOOD:
	{
		return "corpse_food";
	}

	case PickableType::ARMOR:
	{
		return "armor";
	}

	case PickableType::MAGICAL_HELM:
	{
		return "magical_helm";
	}

	case PickableType::MAGICAL_RING:
	{
		return "magical_ring";
	}

	case PickableType::JEWELRY_AMULET:
	{
		return "jewelry_amulet";
	}

	case PickableType::GAUNTLETS:
	{
		return "gauntlets";
	}

	case PickableType::GIRDLE:
	{
		return "girdle";
	}

	case PickableType::QUEST_ITEM:
	{
		return "quest_item";
	}

	case PickableType::DUNGEON_KEY:
	{
		return "dungeon_key";
	}

	}

	return "weapon";
}

inline PickableType parse_pickable_type(std::string_view s)
{
	if (s == "targeted_scroll")
	{
		return PickableType::TARGETED_SCROLL;
	}
	if (s == "teleporter")
	{
		return PickableType::TELEPORTER;
	}
	if (s == "identify_scroll")
	{
		return PickableType::IDENTIFY_SCROLL;
	}
	if (s == "weapon")
	{
		return PickableType::WEAPON;
	}
	if (s == "shield")
	{
		return PickableType::SHIELD;
	}
	if (s == "consumable")
	{
		return PickableType::CONSUMABLE;
	}
	if (s == "gold_coin")
	{
		return PickableType::GOLD_COIN;
	}
	if (s == "food")
	{
		return PickableType::FOOD;
	}
	if (s == "corpse_food")
	{
		return PickableType::CORPSE_FOOD;
	}
	if (s == "armor")
	{
		return PickableType::ARMOR;
	}
	if (s == "magical_helm")
	{
		return PickableType::MAGICAL_HELM;
	}
	if (s == "magical_ring")
	{
		return PickableType::MAGICAL_RING;
	}
	if (s == "jewelry_amulet")
	{
		return PickableType::JEWELRY_AMULET;
	}
	if (s == "gauntlets")
	{
		return PickableType::GAUNTLETS;
	}
	if (s == "girdle")
	{
		return PickableType::GIRDLE;
	}
	if (s == "quest_item")
	{
		return PickableType::QUEST_ITEM;
	}
	if (s == "dungeon_key")
	{
		return PickableType::DUNGEON_KEY;
	}

	throw std::runtime_error(std::format("unknown pickable_type '{}'", s));
}
