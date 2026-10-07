#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "Actor.h"
#include "InventoryData.h"
#include "Item.h"
#include "Vector2D.h"

// Forward declarations
class Creature;
class DataManager;

namespace InventoryOperations
{

// Both FloorInventory and CreatureInventory share the same data shape.
// This concept gates shared operations. Weight-gated add is CreatureInventory-only.
template <typename T>
concept AnyInventory = std::same_as<T, FloorInventory> || std::same_as<T, CreatureInventory>;

// ===== CORE OPERATIONS =====
// Non-template - defined in InventoryOperations.cpp

// Add to floor. No weight check. Use for loot spawns, drops, and corpses.
InventoryResult<bool> add_item(FloorInventory& inventory, std::unique_ptr<Item> item);

// Add to creature backpack. Capacity check only - no weight gate.
// Use for initialization (starting gear, load). Weight gate lives in add_item_to_inventory.
InventoryResult<bool> add_item(CreatureInventory& inventory, std::unique_ptr<Item> item);

// Add to creature backpack with STR-based weight enforcement.
InventoryResult<bool> add_item_to_inventory(
	CreatureInventory& inventory,
	std::unique_ptr<Item> item,
	const Creature& owner,
	const DataManager& dataManager);

// Remove from floor
InventoryResult<std::unique_ptr<Item>> remove_item(FloorInventory& inventory, const Item& item);
InventoryResult<std::unique_ptr<Item>> remove_item_at(FloorInventory& inventory, size_t index);

// Remove from creature backpack
InventoryResult<std::unique_ptr<Item>> remove_item(CreatureInventory& inventory, const Item& item);
InventoryResult<std::unique_ptr<Item>> remove_item_at(CreatureInventory& inventory, size_t index);

// Remove from creature backpack by id
InventoryResult<std::unique_ptr<Item>> remove_item_by_id(CreatureInventory& inventory, uint64_t uniqueId);

// What a purse of the given size weighs, in pounds, at a thousand coins to the pound.
// The part-thousand is dropped, so small change weighs nothing and every further
// thousand costs a pound. Refuses a negative purse: no path can overdraw one, since
// both sides of a trade check the payer can afford it first.
//
// The archive prints no coins-per-pound figure anywhere, so the rate is the owner's
// ruling of 2026-10-03 rather than a rule read off a table. It puts coin at 1,000 gold
// a pound, denser in value than any mundane gear and level with the magical, which is
// what makes a hoard weigh something and pocket money nothing.
//
// Example:
//   coin_weight(999);     // -> 0, pocket change
//   coin_weight(1000);    // -> 1
//   coin_weight(2999);    // -> 2, the part-thousand is dropped
[[nodiscard]] int coin_weight(int goldPieces) noexcept;

// Everything the owner is carrying, in pounds: what is in the pack, what is on the
// body, the purse, and the five pounds the book allows for a character's clothes. It
// totals "the pounds of gear carried by the creature or character... Add five pounds
// for clothing, if any is worn" (Player's Handbook, PDF page 160) and draws no line
// between pack and body, so armour cannot be carried for nothing by putting it on. A
// monster wears no clothing here, Table 47 being Character Encumbrance.
//
// Example:
//   get_total_weight(emptyHandedMonster);          // -> 0
//   get_total_weight(emptyHandedRogue);            // -> 5, the clothes it stands in
//   get_total_weight(plateMailedSwordsman);        // -> 59, the 50 worn, the 4 held and the 5
//   get_total_weight(rogueCarryingThreeThousand);  // -> 8, the clothes and three pounds of coin
int get_total_weight(const Creature& owner) noexcept;

// The most the owner can carry and still move, in pounds: the Strength row's
// maxCarried, which is Table 47's Max. Carried Weight. Exceptional Strength is
// resolved, so an 18/00 fighter is answered from its own band.
//
// Example:
//   get_max_weight(strengthTenCarrier, dataManager);   // -> 110
//   get_max_weight(hillGiantGirdled, dataManager);     // -> 640, at Strength 19
int get_max_weight(const Creature& owner, const DataManager& dataManager) noexcept;

// Whether the owner is carrying more than its Max. Carried Weight, worn gear
// included. Being over it stops the owner picking anything else up; this game has no
// movement rate for Table 48 to reduce, so that is the whole of what it costs. The
// same column answers encumbrance_band's OVERLOADED, so the panel's word and this
// cannot disagree at a Strength whose graded bands are blank.
//
// Example, the calls CarryWeightTest.WhatIsWornCountsAgainstTheLimit makes:
//   carrier.wear(weighing(limit + 1), EquipmentSlot::BODY);  // worn, not packed
//   is_overloaded(carrier, dataManager);                     // -> true
//   is_within_weight_limit(*weighing(1), carrier, dataManager);  // -> false, nothing more fits
bool is_overloaded(const Creature& owner, const DataManager& dataManager) noexcept;

// Whether this item can go into the pack without taking what the owner carries past
// its strength-based limit. The one statement of that rule: adding, picking up
// and buying all ask here, so they cannot disagree about what is too heavy.
//
// Example:
//   is_within_weight_limit(dagger, player, dataManager);      // -> true
//   is_within_weight_limit(plateArmour, weakling, dataManager); // -> false
bool is_within_weight_limit(
	const Item& item,
	const Creature& owner,
	const DataManager& dataManager) noexcept;

// Whether the pack holds an item registered under this key. The key is the one in
// data/content/items.json, so a caller names what it is looking for the same way the
// data does rather than by display name, which is localised and can repeat.
//
// Example:
//   is_carrying(emptyPack, "torch");        // -> false
//   is_carrying(packHoldingATorch, "torch"); // -> true
[[nodiscard]] bool is_carrying(const CreatureInventory& inventory, std::string_view itemKey) noexcept;

// id-based search - creature backpack only
Item* find_item_by_id(CreatureInventory& inventory, uint64_t uniqueId) noexcept;
const Item* find_item_by_id(const CreatureInventory& inventory, uint64_t uniqueId) noexcept;

// ===== CAPACITY MANAGEMENT =====

template <AnyInventory T>
bool is_inventory_full(const T& inventory) noexcept
{
	return inventory.items.size() >= inventory.capacity;
}

template <AnyInventory T>
bool is_inventory_empty(const T& inventory) noexcept
{
	return inventory.items.empty();
}

template <AnyInventory T>
size_t get_item_count(const T& inventory) noexcept
{
	return inventory.items.size();
}

template <AnyInventory T>
size_t get_remaining_space(const T& inventory) noexcept
{
	return inventory.capacity - inventory.items.size();
}

template <AnyInventory T>
void set_inventory_capacity(T& inventory, size_t newCapacity)
{
	if (newCapacity < inventory.items.size())
	{
		inventory.items.resize(newCapacity);
	}

	inventory.capacity = newCapacity;
	inventory.items.reserve(inventory.capacity);

	if (inventory.eventHandler)
	{
		InventoryEvent event{
			.type = InventoryEvent::Type::CAPACITY_CHANGED,
			.item = nullptr,
			.currentSize = inventory.items.size(),
			.capacity = inventory.capacity
		};
		inventory.eventHandler(event);
	}
}

// ===== ITEM ACCESS =====

template <AnyInventory T>
Item* get_item_at(T& inventory, size_t index) noexcept
{
	return index < inventory.items.size() ? inventory.items[index].get() : nullptr;
}

template <AnyInventory T>
const Item* get_item_at(const T& inventory, size_t index) noexcept
{
	return index < inventory.items.size() ? inventory.items[index].get() : nullptr;
}

// ===== SEARCH OPERATIONS =====

template <AnyInventory T>
const Item* find_item_by_name(const T& inventory, std::string_view name) noexcept
{
	[[maybe_unused]] auto is_null = [](const auto& item)
	{
		return !item;
	};
	assert(std::ranges::none_of(inventory.items, is_null));

	auto it = std::ranges::find_if(inventory.items,
		[name](const auto& item)
		{
			return item->actorData.name == name;
		});

	return it != inventory.items.end() ? it->get() : nullptr;
}

template <AnyInventory T>
bool contains_item(const T& inventory, const Item& item) noexcept
{
	return std::ranges::any_of(inventory.items,
		[&item](const auto& storedItem)
		{
			return storedItem.get() == &item;
		});
}

// ===== EVENT SYSTEM =====

template <AnyInventory T>
void set_inventory_event_handler(T& inventory, InventoryEventHandler handler)
{
	inventory.eventHandler = std::move(handler);
}

template <AnyInventory T>
void fire_inventory_event(T& inventory, InventoryEvent::Type type, const Item* item = nullptr)
{
	if (inventory.eventHandler)
	{
		InventoryEvent event{
			.type = type,
			.item = item,
			.currentSize = inventory.items.size(),
			.capacity = inventory.capacity
		};
		inventory.eventHandler(event);
	}
}

// ===== PERSISTENCE =====

template <AnyInventory T>
void load_inventory(T& inventory, const nlohmann::json& j)
{
	inventory.capacity = j.at("capacity").get<size_t>();
	inventory.items.clear();
	inventory.items.reserve(inventory.capacity);

	// A broken item record throws out of here: catching it would hand back the items that
	// came before it and silently drop the rest.
	for (const auto& itemJson : j.at("inventory"))
	{
		auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{});
		item->load(itemJson);
		inventory.items.push_back(std::move(item));
	}
}

// Writes the capacity and one record per item, in the order the inventory holds them.
// An inventory owns everything in it, so a null entry is a fault rather than an empty
// slot: skipping one would write a record shorter than the inventory and the next load
// would hand back fewer items with nothing said. load_inventory already refuses to lose
// items that way, and this is the same rule facing the other direction.
//
// Example:
//
//   nlohmann::json record;
//   save_inventory(pack, record);        // pack: 3 items, capacity 50
//   record["inventory"].size();          // -> 3
//   record["capacity"];                  // -> 50
//
//   save_inventory(emptyPack, record);   // an empty inventory is legal
//   record["inventory"].size();          // -> 0
template <AnyInventory T>
void save_inventory(const T& inventory, nlohmann::json& j)
{
	j["capacity"] = inventory.capacity;
	j["inventory"] = nlohmann::json::array();

	for (const auto& item : inventory.items)
	{
		assert(item && "save_inventory: inventory holds a null where an item should be");

		nlohmann::json itemJson;
		item->save(itemJson);
		j["inventory"].push_back(itemJson);
	}
}

// ===== DEBUG =====

template <AnyInventory T>
std::string get_inventory_debug_info(const T& inventory)
{
	return std::format(
		"Inventory{{items:{}, capacity:{}, full:{}}}",
		get_item_count(inventory),
		inventory.capacity,
		is_inventory_full(inventory) ? "yes" : "no");
}

// ===== OPTIMIZATION =====

template <AnyInventory T>
void optimize_inventory_storage(T& inventory)
{
	std::erase_if(inventory.items, [](const auto& item)
		{ return !item; });

	if (inventory.items.size() * 4 < inventory.items.capacity())
	{
		inventory.items.shrink_to_fit();
	}
}

} // namespace InventoryOperations
