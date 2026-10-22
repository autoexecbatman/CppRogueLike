#pragma once

#include <algorithm>
#include <array>
#include <cassert>

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>

#include <variant>

#include "BuffType.h"
#include "CorpseEdibility.h"
#include "DamageInfo.h"
#include "DiceExpr.h"
#include "EquipmentSlot.h"
#include "MagicalItemEffects.h"
#include "Persistent.h"
#include "TargetMode.h"
#include "TurnSchedule.h"
#include "Vector2D.h"
#include "Weapons.h"

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

// Every PickableType, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<PickableType, 17> ALL_PICKABLE_TYPE = {
	PickableType::TARGETED_SCROLL,
	PickableType::TELEPORTER,
	PickableType::WEAPON,
	PickableType::SHIELD,
	PickableType::CONSUMABLE,
	PickableType::GOLD_COIN,
	PickableType::FOOD,
	PickableType::CORPSE_FOOD,
	PickableType::ARMOR,
	PickableType::MAGICAL_HELM,
	PickableType::MAGICAL_RING,
	PickableType::JEWELRY_AMULET,
	PickableType::GAUNTLETS,
	PickableType::GIRDLE,
	PickableType::QUEST_ITEM,
	PickableType::IDENTIFY_SCROLL,
	PickableType::DUNGEON_KEY,
};

// The next PickableType in that order, wrapping at the end.
//
// Example:
//   next_pickable_type(PickableType::TARGETED_SCROLL);   // -> PickableType::TELEPORTER
[[nodiscard]] inline PickableType next_pickable_type(PickableType value)
{
	const auto found = std::ranges::find(ALL_PICKABLE_TYPE, value);
	if (found == ALL_PICKABLE_TYPE.end() || found + 1 == ALL_PICKABLE_TYPE.end())
	{
		return ALL_PICKABLE_TYPE.front();
	}
	return *(found + 1);
}

// Effect type for Consumable
enum class ConsumableEffect
{
	NONE,
	HEAL,
	ADD_BUFF,
	FAIL,
};

// Every ConsumableEffect, in the order the enum declares them, so a cycle through the
// editor's field reaches all of them and adding one is a single edit here.
inline constexpr std::array<ConsumableEffect, 4> ALL_CONSUMABLE_EFFECT = {
	ConsumableEffect::NONE,
	ConsumableEffect::HEAL,
	ConsumableEffect::ADD_BUFF,
	ConsumableEffect::FAIL,
};

// The next ConsumableEffect in that order, wrapping at the end.
//
// Example:
//   next_consumable_effect(ConsumableEffect::NONE);   // -> ConsumableEffect::HEAL
[[nodiscard]] inline ConsumableEffect next_consumable_effect(ConsumableEffect value)
{
	const auto found = std::ranges::find(ALL_CONSUMABLE_EFFECT, value);
	if (found == ALL_CONSUMABLE_EFFECT.end() || found + 1 == ALL_CONSUMABLE_EFFECT.end())
	{
		return ALL_CONSUMABLE_EFFECT.front();
	}
	return *(found + 1);
}

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

// Whether a helm that answers to a command word has been spoken to since it went on.
// The helm of brilliance "functions only upon the utterance of a special command word"
// (DMG, PDF page 968), and taking it off sends it dormant again (the owner, 2026-10-21).
enum class HelmCommand
{
	DORMANT,
	EMPOWERED
};

// Every HelmCommand, in the order the enum declares them.
inline constexpr std::array<HelmCommand, 2> ALL_HELM_COMMAND = {
	HelmCommand::DORMANT,
	HelmCommand::EMPOWERED,
};

// The name a record carries for this state.
//
// Example:
//   encode_helm_command(HelmCommand::EMPOWERED); // -> "empowered"
inline std::string_view encode_helm_command(HelmCommand helmCommand)
{
	switch (helmCommand)
	{
	case HelmCommand::DORMANT:
	{
		return "dormant";
	}
	case HelmCommand::EMPOWERED:
	{
		return "empowered";
	}
	}

	return "dormant";
}

// The state a record names. Throws naming what it read.
//
// Example:
//   parse_helm_command("empowered");  // -> HelmCommand::EMPOWERED
//   parse_helm_command("awake");      // throws std::runtime_error
inline HelmCommand parse_helm_command(std::string_view name)
{
	if (name == "dormant")
	{
		return HelmCommand::DORMANT;
	}
	if (name == "empowered")
	{
		return HelmCommand::EMPOWERED;
	}

	throw std::runtime_error(std::format("unknown helm command '{}'", name));
}

// Every unit a duration can be written in.
inline constexpr std::array<DurationUnit, 2> ALL_DURATION_UNIT = {
	DurationUnit::ROUNDS,
	DurationUnit::TURNS,
};

// The name a record holds for a duration's unit.
//
// Example:
//   encode_duration_unit(DurationUnit::TURNS);   // -> "turns"
inline std::string_view encode_duration_unit(DurationUnit unit)
{
	switch (unit)
	{
	case DurationUnit::ROUNDS:
	{
		return "rounds";
	}
	case DurationUnit::TURNS:
	{
		return "turns";
	}
	}

	assert(false && "encode_duration_unit: a DurationUnit with no name");
	return "rounds";
}

// The unit a record names. Throws naming what it read.
//
// Example:
//   parse_duration_unit("turns");   // -> DurationUnit::TURNS
//   parse_duration_unit("hours");   // throws std::runtime_error
inline DurationUnit parse_duration_unit(std::string_view name)
{
	if (name == "rounds")
	{
		return DurationUnit::ROUNDS;
	}
	if (name == "turns")
	{
		return DurationUnit::TURNS;
	}

	throw std::runtime_error(std::format("unknown duration unit '{}'", name));
}

// The unit after this one, wrapping, for the editor's toggle.
//
// Example:
//   next_duration_unit(DurationUnit::ROUNDS);   // -> DurationUnit::TURNS
//   next_duration_unit(DurationUnit::TURNS);    // -> DurationUnit::ROUNDS
[[nodiscard]] inline DurationUnit next_duration_unit(DurationUnit unit)
{
	return unit == DurationUnit::ROUNDS ? DurationUnit::TURNS : DurationUnit::ROUNDS;
}

// ========== Plain data structs (no base class, no virtuals) ==========

struct Consumable
{
	ConsumableEffect effect{ ConsumableEffect::NONE };
	// The hit points healed or the buff's value, rolled once when used.
	DiceExpr amount{};
	// The buff's duration, rolled once when drunk, in durationUnit.
	DiceExpr duration{};
	// Whether that duration counts rounds or turns.
	DurationUnit durationUnit{ DurationUnit::ROUNDS };
	BuffType buffType{ BuffType::NONE };
	BuffApplication application{ BuffApplication::ADD };
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
	EquipmentSlot get_preferred_slot(const Creature* wearer) const;
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
	// The effect's duration, rolled once when read, in durationUnit.
	DiceExpr duration{};
	// Whether that duration counts rounds or turns.
	DurationUnit durationUnit{ DurationUnit::ROUNDS };
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
	// The line printed when it is eaten, or when eating it is refused.
	std::string taste{};
	CorpseEdibility edibility{ CorpseEdibility::EDIBLE };
};

struct Armor
{
	int armorClass{ 0 };
};

struct MagicalHelm
{
	MagicalEffect effect{ MagicalEffect::NONE };
	int bonus{ 0 };
	// Read only by a helm that answers to a command word; every other helm works as worn.
	HelmCommand command{ HelmCommand::DORMANT };
	// The game time a once-a-day power is usable again; read only by a helm of teleportation.
	int rechargedAt{ 0 };
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
	BuffApplication application{ BuffApplication::ADD };
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
	BuffApplication application{ BuffApplication::ADD };
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
	BuffApplication application{ BuffApplication::ADD };
	// Exceptional Strength a setting item gives with an 18, 1-100 with 100 as 18/00.
	int exceptionalStrength{ 0 };
};

struct Amulet
{
};

// Used on a locked door via bump - consumed automatically by PlayerController.
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

// Whether this is a helm whose command word is known and not yet spoken: a helm of
// brilliance, its type identified, and dormant. Identifying it is how the word is learned
// (the owner, 2026-10-21: "Must be discovered"), so an unidentified one is never ready.
//
// Example:
//   is_awaiting_command_word(identifiedDormantBrilliance);    // -> true
//   is_awaiting_command_word(unidentifiedBrilliance);         // -> false, the word is unknown
//   is_awaiting_command_word(identifiedEmpoweredBrilliance);  // -> false, already spoken
//   is_awaiting_command_word(helmOfTeleportation);            // -> false, it has no word
[[nodiscard]] bool is_awaiting_command_word(const Item& item);

// Speaks the command word to the helm the wearer has on: it is empowered, and its armour
// and fire protection count from now on. The caller spends the turn. The helm must be
// worn on the wearer's head and awaiting its word.
//
// Example:
//   speak_command_word(helm, player, ctx);   // AC 10 -> 8, "The helm of brilliance blazes..."
void speak_command_word(Item& helm, Creature& wearer, GameContext& ctx);

// Ends whatever a worn item was empowered to do, as it comes off: a spoken helm goes
// dormant. Anything else is left as it is.
//
// Example:
//   make_dormant(empoweredBrilliance);   // its command -> HelmCommand::DORMANT
//   make_dormant(longSword);             // unchanged
void make_dormant(Item& item) noexcept;

// Whether a helm of teleportation can teleport its wearer at game time `now`: it is used
// "once per day" (DMG, PDF page 970), so a day must have passed since its last use. Any
// other item has no daily power and is never ready.
//
// Example:
//   is_helm_ready(unusedTeleportation, 5000);          // -> true
//   record_helm_use(unusedTeleportation, 5000);        // usable again at 177800
//   is_helm_ready(unusedTeleportation, 177799);        // -> false, a time unit short
//   is_helm_ready(unusedTeleportation, 177800);        // -> true
//   is_helm_ready(helmOfBrilliance, 5000);             // -> false, it has no daily power
[[nodiscard]] bool is_helm_ready(const Item& helm, int now);

// Spends a helm of teleportation's use at game time `now`, so it is ready again a day
// later. The helm must be a helm of teleportation.
//
// Example:
//   record_helm_use(helm, 5000);   // its rechargedAt -> 177800
void record_helm_use(Item& helm, int now);

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

bool use(Consumable& consumable, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Weapon& weapon, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Shield& shield, Item& owner, Creature& wearer, GameContext& ctx);
bool use(TargetedScroll& targetScroll, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Teleporter& teleporter, Item& owner, Creature& wearer, GameContext& ctx);
bool use(IdentifyScroll& identifyScroll, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Gold& gold, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Food& food, Item& owner, Creature& wearer, GameContext& ctx);
bool use(CorpseFood& corpseFood, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Armor& armor, Item& item, Creature& wearer, GameContext& ctx);
bool use(MagicalHelm& magicalHelm, Item& owner, Creature& wearer, GameContext& ctx);
bool use(MagicalRing& magicalRing, Item& owner, Creature& wearer, GameContext& ctx);
bool use(JewelryAmulet& jewelryAmulet, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Gauntlets& gauntlets, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Girdle& girdle, Item& owner, Creature& wearer, GameContext& ctx);
bool use(Amulet& amulet, Item& owner, Creature& wearer, GameContext& ctx);
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

bool use_item(ItemBehavior& behavior, Item& owner, Creature& wearer, GameContext& ctx);
int get_item_ac_bonus(const ItemBehavior& behavior) noexcept;

// Whether using this item puts its user somewhere else. Asked before the use, because the
// use consumes a scroll: a scroll of teleportation moves its reader, nothing else does.
//
// Example:
//   moves_its_user(ItemBehavior{ Teleporter{} });     // -> true
//   moves_its_user(ItemBehavior{ IdentifyScroll{} }); // -> false
bool moves_its_user(const ItemBehavior& behavior) noexcept;

// Whether using this item acts on creatures the reader picks out or can see. Every
// targeted scroll does: sleep, hold person, lightning, confusion and fireball.
//
// Example:
//   targets_creatures(ItemBehavior{ TargetedScroll{} }); // -> true
//   targets_creatures(ItemBehavior{ Teleporter{} });     // -> false
bool targets_creatures(const ItemBehavior& behavior) noexcept;

// Whether using this item hands the reader to a targeting cursor that finishes the use
// later: a targeted scroll aimed at a tile, which is confusion and fireball. The use
// reports nothing consumed when it returns, because the cursor has not closed yet.
//
// Example:
//   hands_over_to_targeting(ItemBehavior{ TargetedScroll{ .targetMode = TargetMode::PICK_TILE_AOE } }); // -> true
//   hands_over_to_targeting(ItemBehavior{ TargetedScroll{ .targetMode = TargetMode::FOV_BUFF } });      // -> false
bool hands_over_to_targeting(const ItemBehavior& behavior) noexcept;

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
void save_behavior(const ItemBehavior& behavior, json& output);
ItemBehavior load_behavior(const json& source);

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
