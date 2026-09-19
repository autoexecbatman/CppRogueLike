#pragma once

#include <map>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "BuffType.h"
#include "ItemClassification.h"
#include "MagicalItemEffects.h"
#include "Pickable.h"
#include "TargetMode.h"
#include "Weapons.h"

// file: ItemRegistry.h
//
// Every item the game knows, by key - the items items.json ships and the ones the item
// editor adds - and the rules for spawning enhanced weapons and armour. Game owns the
// registry the game plays with and hands it out as ctx.itemRegistry; ItemCreator builds
// items from what it holds and ItemFactory draws spawns from it. Each registry is its
// own value, so a test builds one and an edit to it reaches nothing else.
//
// Usage:
//
//   ItemRegistry items;
//   items.load("data/content/items.json"); // throws if missing, or a record lacks a field
//   items.load_enhanced_rules("data/content/enhanced_rules.json");
//   items.get_params("health_potion").value; // -> 50
//   items.add_custom("Registry Trinket", "potion", ItemParams{}); // -> "registry_trinket"
//   items.remove_custom("health_potion"); // throws std::logic_error, a shipped item
//   items.get_params("no_such_item"); // throws std::out_of_range
//   items.save("data/content/items.json");

// Unified compositional item parameters
struct ItemParams
{
	// Display & Classification
	// The name every menu shows - a view into the string the registry owns.
	std::string_view name{ "" };
	// The colour pair it is drawn in.
	int color{ 0 };
	// What kind of item it is, for identification and a mimic's disguise.
	ItemClass itemClass{ ItemClass::UNKNOWN };
	// What it is worth in gold.
	int value{ 0 };
	// Which behaviour it is given, and so what using it does.
	PickableType pickableType{ PickableType::WEAPON };

	// A consumable's amount - the hit points healed, or the buff's strength.
	int consumableAmount{ 0 };
	// A targeted scroll's reach.
	int range{ 0 };
	// A targeted scroll's damage.
	int damage{ 0 };
	// How many turns a targeted scroll's confusion lasts.
	int confuseTurns{ 0 };
	// How many turns a consumable's or targeted scroll's effect lasts.
	int duration{ 0 };

	// A magical helm's, ring's or ability item's effect.
	MagicalEffect effect{ MagicalEffect::NONE };
	// The strength of that effect.
	int effectBonus{ 0 };

	// What an ability item does to Strength.
	int strBonus{ 0 };
	// What an ability item does to Dexterity.
	int dexBonus{ 0 };
	// What an ability item does to Constitution.
	int conBonus{ 0 };
	// What an ability item does to Intelligence.
	int intBonus{ 0 };
	// What an ability item does to Wisdom.
	int wisBonus{ 0 };
	// What an ability item does to Charisma.
	int chaBonus{ 0 };
	// Whether those bonuses, and a consumable's amount, set a score rather than add to it.
	bool isSetMode{ false };
	// The 18/xx percentile an item setting Strength to 18 gives.
	int exceptionalStrength{ 0 };

	// What food restores against hunger.
	int nutritionValue{ 0 };
	// What armour takes off armour class.
	int acBonus{ 0 };

	// Whether a weapon fires from the missile slot.
	bool ranged{ false };
	// How many hands a weapon needs.
	HandRequirement handRequirement{ HandRequirement::ONE_HANDED };
	// A weapon's size.
	WeaponSize weaponSize{ WeaponSize::MEDIUM };

	// What a consumable does.
	ConsumableEffect consumableEffect{ ConsumableEffect::NONE };
	// Which buff a consumable or targeted scroll grants.
	BuffType consumableBuffType{ BuffType::INVISIBILITY };

	// How a targeted scroll chooses its target.
	TargetMode targetMode{ TargetMode::AUTO_NEAREST };
	// What a targeted scroll shows when read.
	ScrollAnimation scrollAnimation{ ScrollAnimation::NONE };

	// How often this item is drawn from its category. Not its mass.
	int baseWeight{ 0 };
	// What it weighs in a pack, against the carrier's strength-derived cap.
	int weight{ 0 };
	// The first dungeon level it can spawn on.
	int levelMin{ 1 };
	// The last dungeon level it can spawn on; 0 means no limit.
	int levelMax{ 0 };
	// How much its spawn weight grows per level past the first; negative shrinks it.
	float levelScaling{ 0.0f };
	// The spawn category it is drawn under - a view into the string the registry owns.
	std::string_view category{ "" };
};

// Which enhancement an enhanced spawn rule gives the item it draws.
enum class EnhancedItemCategory
{
	WEAPON,
	ARMOR
};

// One rule for spawning an enhanced weapon or armour: a pool of item keys to draw from,
// and the spawn weight and levels the rule itself carries.
struct EnhancedItemSpawnRule
{
	// The item keys one is drawn from.
	std::vector<std::string> itemPool;
	// Whether it gets a weapon or an armour enhancement.
	EnhancedItemCategory enhancementCategory{};
	// How often the rule is drawn.
	int baseWeight{ 0 };
	// The first dungeon level it can spawn on.
	int levelMin{ 0 };
	// The last dungeon level it can spawn on; 0 means no limit.
	int levelMax{ 0 };
	// How much its weight grows per level past the first.
	float levelScaling{ 0.0f };
	// The spawn category it is drawn under.
	std::string category;
};

// The shipped and custom items and the enhanced spawn rules; see the file comment above
// for use. It cannot be copied: each item's name and category are views into strings
// its entries own, so a copy's views would point into the original.
class ItemRegistry
{
private:
	// One item: the name and category strings its params' views point into.
	struct ItemEntry
	{
		std::string name{};
		std::string category{};
		ItemParams params{};
	};

	// Every item by key. Map nodes do not move, so the views into them stay valid.
	std::map<std::string, ItemEntry, std::less<>> entries{};
	// The keys items.json shipped, which cannot be removed.
	std::unordered_set<std::string> builtinKeys{};
	// The rules for enhanced weapons and armour.
	std::vector<EnhancedItemSpawnRule> enhancedRules{};

	// Points an entry's name and category views at the strings it owns. Run whenever an
	// entry's strings or params are written.
	static void point_views_at_own_strings(ItemEntry& entry);

public:
	// Holds no items and no rules until load and load_enhanced_rules.
	ItemRegistry() = default;
	~ItemRegistry() = default;
	ItemRegistry(const ItemRegistry&) = delete;
	ItemRegistry& operator=(const ItemRegistry&) = delete;
	ItemRegistry(ItemRegistry&&) = delete;
	ItemRegistry& operator=(ItemRegistry&&) = delete;

	// Reads every item, replacing all held, custom ones included. A record missing any
	// field throws, naming the item and the field.
	void load(std::string_view path);

	// Writes every item, shipped and custom.
	void save(std::string_view path) const;

	// Every key, in key order.
	[[nodiscard]] std::vector<std::string> get_all_keys() const;

	// Throws std::out_of_range if key is unknown.
	[[nodiscard]] const ItemParams& get_params(std::string_view key) const;

	// Replaces an item's parameters, keeping its own name and category. Throws
	// std::out_of_range if key is unknown.
	void set_params(std::string_view key, const ItemParams& params);

	// Renames and recategorises an item. Throws std::out_of_range if key is unknown.
	void set_name_category(std::string_view key, std::string name, std::string category);

	// Adds a user-created item under a key made from its name, numbered past any key
	// already taken, and returns that key.
	[[nodiscard]] std::string add_custom(std::string name, std::string category, ItemParams params);

	// Removes a user-created item. Throws std::logic_error for a shipped item and
	// std::out_of_range for an unknown key.
	void remove_custom(std::string_view key);

	// Whether a key names an item items.json shipped.
	[[nodiscard]] bool is_builtin_key(std::string_view key) const;

	// Reads the enhanced spawn rules, replacing those held. An unknown enhancement
	// category throws.
	void load_enhanced_rules(std::string_view path);

	// The enhanced spawn rules.
	[[nodiscard]] std::span<const EnhancedItemSpawnRule> get_enhanced_rules() const;
};
