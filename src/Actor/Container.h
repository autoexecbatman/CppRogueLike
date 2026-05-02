#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "../Persistent/Persistent.h"

// Forward declarations
class Actor;
class Item;

enum class ContainerError
{
	FULL,
	ITEM_NOT_FOUND,
	INVALID_ITEM,
	CAPACITY_EXCEEDED
};

template <typename T>
using ContainerResult = std::expected<T, ContainerError>;

// Event system for decoupled communication
struct ContainerEvent
{
	enum class Type
	{
		ITEM_ADDED,
		ITEM_REMOVED,
		INVENTORY_FULL,
		CAPACITY_CHANGED
	};
	Type type{ Type::ITEM_ADDED };
	const Item* item{ nullptr };
	size_t currentSize{ 0 };
	size_t capacity{ 0 };
};

using ContainerEventHandler = std::function<void(const ContainerEvent&)>;

// - Modern C++20 inventory management with proper encapsulation
class Container : public Persistent
{
private:
	// Private data members - proper encapsulation
	std::vector<std::unique_ptr<Item>> inventory;
	size_t capacity{};
	int maxWeight{ 500 }; // Roughly 250 lbs in D&D scaling (2 units per lb)
	ContainerEventHandler eventHandler;

	// Internal helper methods
	void fire_event(ContainerEvent::Type type, const Item* item = nullptr);
	void optimize_storage();

public:
	explicit Container(size_t initialCapacity) noexcept;
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
	[[nodiscard]] const std::vector<std::unique_ptr<Item>>& get_inventory() const noexcept { return inventory; }
	[[nodiscard]] std::vector<std::unique_ptr<Item>>& get_inventory_mutable() noexcept { return inventory; }

	[[nodiscard]] size_t get_capacity() const noexcept { return capacity; }
	[[nodiscard]] size_t get_item_count() const noexcept { return inventory.size(); }
	[[nodiscard]] bool is_full() const noexcept { return inventory.size() >= capacity; }
	[[nodiscard]] bool is_empty() const noexcept { return inventory.empty(); }
	[[nodiscard]] size_t get_remaining_space() const noexcept { return capacity - inventory.size(); }

	// Weight management
	[[nodiscard]] int get_total_weight() const noexcept;
	[[nodiscard]] int get_max_weight() const noexcept { return maxWeight; }
	[[nodiscard]] bool is_overloaded() const noexcept { return get_total_weight() > maxWeight; }
	[[nodiscard]] bool can_run() const noexcept { return !is_overloaded(); }
	void set_max_weight(int max_w) noexcept { maxWeight = max_w; }

	// Item access
	[[nodiscard]] Item* get_item_at(size_t index) noexcept;
	[[nodiscard]] const Item* get_item_at(size_t index) const noexcept;

	// Capacity management
	void set_capacity(size_t newCapacity);

	// Search operations
	[[nodiscard]] const Item* find_item_by_name(std::string_view name) const noexcept;
	[[nodiscard]] bool contains_item(const Item& item) const noexcept;

	// Event system for decoupled communication
	void set_event_handler(ContainerEventHandler handler) { eventHandler = std::move(handler); }

	// Persistence interface
	void load(const json& j) override;
	void save(json& j) override;

	// Debug utilities
	void print_container(std::span<std::unique_ptr<Actor>> container) const;
	[[nodiscard]] std::string get_debug_info() const;

};
