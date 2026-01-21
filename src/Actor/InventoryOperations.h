#pragma once

#include <memory>
#include <string_view>
#include <span>

#include <nlohmann/json.hpp>

#include "InventoryData.h"

// Forward declarations
class Item;
class Actor;
using json = nlohmann::json;

// - Free functions for inventory management
namespace InventoryOperations
{

// ===== CORE OPERATIONS =====

// Add item with proper error handling and capacity constraints
InventoryResult<bool> add_item(InventoryData& inventory, std::unique_ptr<Item> item);

// Remove operations
InventoryResult<std::unique_ptr<Item>> remove_item(InventoryData& inventory, const Item& item);
InventoryResult<std::unique_ptr<Item>> remove_item_at(InventoryData& inventory, size_t index);

// ===== CAPACITY MANAGEMENT =====

// Const-correct capacity queries
bool is_inventory_full(const InventoryData& inventory) noexcept;
bool is_inventory_empty(const InventoryData& inventory) noexcept;
size_t get_item_count(const InventoryData& inventory) noexcept;
size_t get_remaining_space(const InventoryData& inventory) noexcept;

// Capacity modification
void set_inventory_capacity(InventoryData& inventory, size_t new_capacity);

// ===== ITEM ACCESS =====

// Const-correct item access
Item* get_item_at(InventoryData& inventory, size_t index) noexcept;
const Item* get_item_at(const InventoryData& inventory, size_t index) noexcept;

// ===== SEARCH OPERATIONS =====

// Const-correct search functions
const Item* find_item_by_name(const InventoryData& inventory, std::string_view name) noexcept;
Item* find_item_by_id(InventoryData& inventory, uint64_t unique_id) noexcept;
const Item* find_item_by_id(const InventoryData& inventory, uint64_t unique_id) noexcept;
InventoryResult<std::unique_ptr<Item>> remove_item_by_id(InventoryData& inventory, uint64_t unique_id);
bool contains_item(const InventoryData& inventory, const Item& item) noexcept;

// ===== EVENT SYSTEM =====

// Event management
void set_inventory_event_handler(InventoryData& inventory, InventoryEventHandler handler);
void fire_inventory_event(InventoryData& inventory, InventoryEvent::Type type, const Item* item = nullptr);

// ===== PERSISTENCE =====

// JSON load/save operations
void load_inventory(InventoryData& inventory, const json& j);
void save_inventory(const InventoryData& inventory, json& j);

// ===== DEBUG UTILITIES =====

// Debug and display functions
void print_inventory(const InventoryData& inventory, std::span<std::unique_ptr<Actor>> actors);
std::string get_inventory_debug_info(const InventoryData& inventory);

// ===== OPTIMIZATION =====

// Internal optimization (private-like function)
void optimize_inventory_storage(InventoryData& inventory);

} // namespace InventoryOperations
