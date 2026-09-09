#include <format>
#include <fstream>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

#include "../Core/Paths.h"
#include "BodyPlanRegistry.h"

void BodyPlanRegistry::load(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream file(resolved);
	if (!file.is_open())
	{
		throw std::runtime_error(
			std::format("BodyPlanRegistry::load -- cannot open '{}'", resolved.string()));
	}

	nlohmann::json root = nlohmann::json::parse(file);

	plans.clear();

	for (const auto& [templateName, slotNames] : root.items())
	{
		std::vector<EquipmentSlot> slots;
		slots.reserve(slotNames.size());
		for (const nlohmann::json& slotName : slotNames)
		{
			slots.push_back(parse_equipment_slot(slotName.get<std::string>()));
		}
		plans.emplace(templateName, std::move(slots));
	}
}

const std::vector<EquipmentSlot>& BodyPlanRegistry::get(std::string_view name) const
{
	// A creature that wears nothing names no template.
	if (name.empty())
	{
		return nothing;
	}

	auto found = plans.find(std::string{ name });
	if (found == plans.end())
	{
		throw std::runtime_error(std::format("unknown body plan '{}'", name));
	}

	return found->second;
}
