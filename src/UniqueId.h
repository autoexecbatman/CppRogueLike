#pragma once

#include <atomic>
#include <cstdint>

#include <nlohmann/json_fwd.hpp>

// - Unique ID system for game objects
namespace UniqueId
{
using IdType = uint64_t;

// Thread-safe unique ID generator
class Generator
{
private:
	static std::atomic<IdType> nextId;

public:
	static IdType generate()
	{
		return nextId.fetch_add(1, std::memory_order_relaxed);
	}

	// For save/load - set the next ID to resume from
	static void set_next_id(IdType id)
	{
		nextId.store(id, std::memory_order_release);
	}

	static IdType peek_next_id()
	{
		return nextId.load(std::memory_order_acquire);
	}
};

// Invalid/null ID constant
constexpr IdType INVALID_ID = 0;

// Writes the counter into a save, and reads it back out.
//
// A load builds every actor before overwriting its id from the file, so it burns one
// id per surviving actor and throws them away. That leaves the counter below the ids
// the save restored, because anything destroyed during play issued an id that no
// longer belongs to anybody. Without this the next object created after a load takes
// an id some live actor already holds.
//
// Example:
//
//   nlohmann::json saved;
//   UniqueId::save(saved);              // stores where the counter stands
//   // ... process restarts, counter is back at 1 ...
//   UniqueId::load(saved);              // resumes from the stored value
//   UniqueId::Generator::generate();    // -> higher than any id the save holds
void save(nlohmann::json& j);
void load(const nlohmann::json& j);
} // namespace UniqueId
