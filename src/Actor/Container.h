// Container.h - Modern C++20 inventory management with proper encapsulation
#ifndef CONTAINER_H
#define CONTAINER_H

#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <system_error>
#include <span>
#include <functional>
#include <concepts>

#include "../Persistent/Persistent.h"

// Forward declarations
class Actor;
class Item;

// C++20 compatible error handling
enum class ContainerError
{
    FULL,
    ITEM_NOT_FOUND,
    INVALID_ITEM,
    CAPACITY_EXCEEDED
};

// Result type for C++20 compatibility
template<typename T>
struct ContainerResult
{
    std::optional<T> value;
    std::optional<ContainerError> error;
    
    // Success constructor
    ContainerResult(T&& val) : value(std::move(val)) {}
    
    // Error constructor  
    ContainerResult(ContainerError err) : error(err) {}
    
    // Convenience methods
    bool has_value() const { return value.has_value(); }
    bool has_error() const { return error.has_value(); }
    explicit operator bool() const { return has_value(); }
    
    T& operator*() { return *value; }
    const T& operator*() const { return *value; }
    
    ContainerError get_error() const { return error.value_or(ContainerError::INVALID_ITEM); }
};

// Event system for decoupled communication
struct ContainerEvent
{
    enum class Type { ITEM_ADDED, ITEM_REMOVED, INVENTORY_FULL, CAPACITY_CHANGED };
    Type type;
    const Item* item;
    size_t current_size;
    size_t capacity;
};

using ContainerEventHandler = std::function<void(const ContainerEvent&)>;

class Container : public Persistent
{
public:
    explicit Container(size_t initial_capacity) noexcept;
    ~Container() = default;

    // Rule of 5 for proper resource management
    Container(const Container&) = delete;
    Container& operator=(const Container&) = delete;
    Container(Container&&) noexcept = default;
    Container& operator=(Container&&) noexcept = default;

    // Modern interface with proper error handling
    [[nodiscard]] ContainerResult<bool> add(std::unique_ptr<Item> item);
    [[nodiscard]] ContainerResult<std::unique_ptr<Item>> remove(const Item& item);
    [[nodiscard]] ContainerResult<std::unique_ptr<Item>> remove_at(size_t index);
    void remove_legacy(std::unique_ptr<Item> actor); // For unusual legacy pattern

    // Const-correct access methods
    [[nodiscard]] const std::vector<std::unique_ptr<Item>>& get_inventory() const noexcept { return inventory_; }
    [[nodiscard]] std::vector<std::unique_ptr<Item>>& get_inventory_mutable() noexcept { return inventory_; }
    
    [[nodiscard]] size_t get_capacity() const noexcept { return capacity_; }
    [[nodiscard]] size_t get_item_count() const noexcept { return inventory_.size(); }
    [[nodiscard]] bool is_full() const noexcept { return inventory_.size() >= capacity_; }
    [[nodiscard]] bool is_empty() const noexcept { return inventory_.empty(); }
    [[nodiscard]] size_t get_remaining_space() const noexcept { return capacity_ - inventory_.size(); }

    // Item access
    [[nodiscard]] Item* get_item_at(size_t index) noexcept;
    [[nodiscard]] const Item* get_item_at(size_t index) const noexcept;

    // Capacity management
    void set_capacity(size_t new_capacity);

    // Search operations
    [[nodiscard]] const Item* find_item_by_name(std::string_view name) const noexcept;
    [[nodiscard]] bool contains_item(const Item& item) const noexcept;

    // Event system for decoupled communication
    void set_event_handler(ContainerEventHandler handler) { event_handler_ = std::move(handler); }

    // Persistence interface
    void load(const json& j) override;
    void save(json& j) override;

    // Debug utilities
    void print_container(std::span<std::unique_ptr<Actor>> container) const;
    [[nodiscard]] std::string get_debug_info() const;

private:
    // Private data members - proper encapsulation
    std::vector<std::unique_ptr<Item>> inventory_;
    size_t capacity_;
    ContainerEventHandler event_handler_;

    // Internal helper methods
    void fire_event(ContainerEvent::Type type, const Item* item = nullptr);
    void optimize_storage();
};

#endif // CONTAINER_H
