// UniqueId.cpp - Implementation of unique ID system
#include <algorithm>
#include <atomic>

#include <nlohmann/json.hpp>

#include "UniqueId.h"

namespace CppRL
{

	namespace UniqueId
	{

		// Start IDs at 1 (0 is reserved for INVALID_ID)
		std::atomic<IdType> Generator::nextId{ 1 };

		void save(nlohmann::json& output)
		{
			output["nextUniqueId"] = Generator::peek_next_id();
		}

		void load(const nlohmann::json& source)
		{
			// Never backwards: loading in a running game finds the counter
			// ahead, and objects in memory hold those ids.
			const IdType stored = source.at("nextUniqueId").get<IdType>();
			Generator::set_next_id(std::max(stored, Generator::peek_next_id()));
		}
	} // namespace UniqueId

} // namespace CppRL
