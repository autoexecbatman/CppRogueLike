#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <functional>

// Forward declarations
class Item;

// - Data structures for inventory management

// Error handling for inventory operations
enum class InventoryError
{
    FULL,
    ITEM_NOT_FOUND,
    INVALID_ITEM,
    CAPACITY_EXCEEDED
};

// Result type for inventory operations
template<typename T>
struct InventoryResult
{
    std::optional<T> value;
    std::optional<InventoryError> error;
    
    // Success constructor
    InventoryResult(T&& val) : value(std::move(val)) {}
    
    // Error constructor  
    InventoryResult(InventoryError err) : error(err) {}
    
    // Convenience methods
    bool has_value() const { return value.has_value(); }
    bool has_error() const { return error.has_value(); }
    explicit operator bool() const { return has_value(); }
    
    T& operator*() { return *value; }
    const T& operator*() const { return *value; }
    
    InventoryError get_error() const { return error.value_or(InventoryError::INVALID_ITEM); }
};

// Event system for decoupled communication
struct InventoryEvent
{
    enum class Type { ITEM_ADDED, ITEM_REMOVED, INVENTORY_FULL, CAPACITY_CHANGED };
    Type type;
    const Item* item;
    size_t current_size;
    size_t capacity;
};

using InventoryEventHandler = std::function<void(const InventoryEvent&)>;

// Pure data structure - no behavior
struct InventoryData
{
    std::vector<std::unique_ptr<Item>> items;
    size_t capacity;
    InventoryEventHandler event_handler;
    
    // Simple constructor
    explicit InventoryData(size_t initial_capacity) 
        : capacity(initial_capacity), event_handler(nullptr)
    {
        items.reserve(initial_capacity);
    }
    
    // Move semantics
    InventoryData(InventoryData&&) noexcept = default;
    InventoryData& operator=(InventoryData&&) noexcept = default;
    
    // No copying - unique ownership
    InventoryData(const InventoryData&) = delete;
    InventoryData& operator=(const InventoryData&) = delete;
};
