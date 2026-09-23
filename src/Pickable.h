#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

#include <variant>

#include "MagicalItemEffects.h"
#include "Weapons.h"
#include "Persistent.h"
#include "BuffType.h"
#include "TargetMode.h"
#include "DamageInfo.h"
#include "EquipmentSlot.h"
#include "Vector2D.h"

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

// The name a record carries for this effect.
//
// Example:
//   encode_consumable_effect(ConsumableEffect::ADD_BUFF); // -> "add_buff"
inline std::string_view encode_consumable_effect(ConsumableEffect consumableEffect)
{
	switch (consumableEffect)
	{
	case ConsumableEffect::NONE:
	{
		return "none";
	}
	case ConsumableEffect::HEAL:
	{
		return "heal";
	}
	case ConsumableEffect::ADD_BUFF:
	{
		return "add_buff";
	}
	case ConsumableEffect::FAIL:
	{
		return "fail";
	}
	}

	return "none";
}

// The effect a record names. Throws naming what it read.
//
// Example:
//   parse_consumable_effect("heal");    // -> ConsumableEffect::HEAL
//   parse_consumable_effect("healng");  // throws std::runtime_error
inline ConsumableEffect parse_consumable_effect(std::string_view name)
{
	if (name == "none")
	{
		return ConsumableEffect::NONE;
	}
	if (name == "heal")
	{
		return ConsumableEffect::HEAL;
	}
	if (name == "add_buff")
	{
		return ConsumableEffect::ADD_BUFF;
	}
	if (name == "fail")
	{
		return ConsumableEffect::FAIL;
	}

	throw std::runtime_error(std::format("unknown consumable effect '{}'", name));
}

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
	// The Strength a bow is specially made for; 0 for an ordinary weapon.
	int strengthRating{ 0 };

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
	// Exceptional Strength a setting item gives with an 18, 1-100 with 100 as 18/00.
	int exceptionalStrength{ 0 };
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
	// Exceptional Strength a setting item gives with an 18, 1-100 with 100 as 18/00.
	int exceptionalStrength{ 0 };
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
	// Exceptional Strength a setting item gives with an 18, 1-100 with 100 as 18/00.
	int exceptionalStrength{ 0 };
};

struct Amulet
{
};

// Used on a locked door via bump — consumed automatically by PlayerController.
// When selected from inventory, shows a hint message.
struct DungeonKey
{
};

// The Strength a weapon is specially made for: a bow of heavier pull, which a weaker arm
// cannot draw (Player's Handbook, PDF page 152) and which gives the Table 1 adjustment of
// its rating. 0 for an ordinary weapon and for anything that is not a weapon.
//
// Example:
//   strength_rating_of(compositeBow);   // -> 18
//   strength_rating_of(longBow);        // -> 0
//   strength_rating_of(healthPotion);   // -> 0
[[nodiscard]] int strength_rating_of(const Item& item);

// Whether the wielder's Strength as it stands now reaches the weapon's rating, which is
// what it takes to use it: "Strength 18 to use it" is the owner's ruling for the composite
// bow. Asked when the weapon is equipped and again each time it is drawn, since Strength
// can fall in between - a girdle of giant strength taken off.
//
// Example:
//   can_draw(strength18Archer, compositeBow);   // -> true
//   can_draw(strength12Archer, compositeBow);   // -> false
//   can_draw(strength3Archer, longBow);         // -> true, an ordinary bow asks nothing
[[nodiscard]] bool can_draw(const Creature& wielder, const Item& weapon);

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

// What reading a single-target scroll at a tile did with the scroll.
enum class ScrollReading
{
	SPENT, // read at the tile: the creature there, if any, took the effect
	KEPT, // not read: the creature there has a Sanctuary that turned the reader away
};

// Reads a confusion scroll at a tile: the creature standing there is confused for the
// given turns. A creature whose Sanctuary turns the reader away is not touched, and the
// scroll stays unread - the reader "totally ignores the warded creature" (PHB page 436).
// An empty tile still spends it.
//
// Example, a goblin on the tile, then a warded one the reader fails to save against:
//   read_confusion_at(player, goblinTile, 8, ctx);   // -> ScrollReading::SPENT, goblin confused
//   read_confusion_at(player, wardedTile, 8, ctx);   // -> ScrollReading::KEPT, untouched
ScrollReading read_confusion_at(const Creature& reader, Vector2D tile, int turns, GameContext& ctx);

// ========== Variant-level dispatchers ==========

bool use_item(ItemBehavior& behavior, Item& owner, Player& wearer, GameContext& ctx);
int get_item_ac_bonus(const ItemBehavior& behavior) noexcept;

// How strongly a worn item's own effect resists the given type, in rings: a
// ring of fire resistance or of warmth at its bonus of 1, the helm of
// brilliance against fire at its bonus of 2 - the book's double-strength ring.
// Zero for everything else, so a sword answers 0.
//
// Example:
//   get_item_resistance_strength(MagicalRing{ MagicalEffect::FIRE_RESISTANCE, 1 }, DamageType::FIRE);  // -> 1
//   get_item_resistance_strength(MagicalRing{ MagicalEffect::FIRE_RESISTANCE, 1 }, DamageType::COLD);  // -> 0
int get_item_resistance_strength(const ItemBehavior& behavior, DamageType damageType) noexcept;

// Serialization
void save_behavior(const ItemBehavior& behavior, json& j);
ItemBehavior load_behavior(const json& j);

inline std::string_view encode_pickable_type(PickableType pickableType)
{
	switch (pickableType)
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

inline PickableType parse_pickable_type(std::string_view name)
{
	if (name == "targeted_scroll")
	{
		return PickableType::TARGETED_SCROLL;
	}
	if (name == "teleporter")
	{
		return PickableType::TELEPORTER;
	}
	if (name == "identify_scroll")
	{
		return PickableType::IDENTIFY_SCROLL;
	}
	if (name == "weapon")
	{
		return PickableType::WEAPON;
	}
	if (name == "shield")
	{
		return PickableType::SHIELD;
	}
	if (name == "consumable")
	{
		return PickableType::CONSUMABLE;
	}
	if (name == "gold_coin")
	{
		return PickableType::GOLD_COIN;
	}
	if (name == "food")
	{
		return PickableType::FOOD;
	}
	if (name == "corpse_food")
	{
		return PickableType::CORPSE_FOOD;
	}
	if (name == "armor")
	{
		return PickableType::ARMOR;
	}
	if (name == "magical_helm")
	{
		return PickableType::MAGICAL_HELM;
	}
	if (name == "magical_ring")
	{
		return PickableType::MAGICAL_RING;
	}
	if (name == "jewelry_amulet")
	{
		return PickableType::JEWELRY_AMULET;
	}
	if (name == "gauntlets")
	{
		return PickableType::GAUNTLETS;
	}
	if (name == "girdle")
	{
		return PickableType::GIRDLE;
	}
	if (name == "quest_item")
	{
		return PickableType::QUEST_ITEM;
	}
	if (name == "dungeon_key")
	{
		return PickableType::DUNGEON_KEY;
	}

	throw std::runtime_error(std::format("unknown pickable_type '{}'", name));
}
