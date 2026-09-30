// UniqueId.cpp - Implementation of unique ID system
#include <algorithm>
#include <atomic>

#include <nlohmann/json.hpp>

#include "UniqueId.h"

namespace UniqueId
{
using json = nlohmann::json;

// Start IDs at 1 (0 is reserved for INVALID_ID)
std::atomic<IdType> Generator::nextId{ 1 };

void save(json& j)
{
	j["nextUniqueId"] = Generator::peek_next_id();
}

void load(const json& j)
{
	if (!j.contains("nextUniqueId"))
	{
		return;
	}

	// Never backwards. Loading a save from inside a running game finds the counter
	// already past what the file holds, and lowering it would hand out ids that
	// objects still in memory are using.
	const IdType stored = j["nextUniqueId"].get<IdType>();
	Generator::set_next_id(std::max(stored, Generator::peek_next_id()));
}
} // namespace UniqueId
