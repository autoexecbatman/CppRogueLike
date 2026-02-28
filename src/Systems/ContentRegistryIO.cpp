// file: ContentRegistryIO.cpp

#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "../Core/Paths.h"
#include "../Factories/MonsterCreator.h"
#include "../Items/ItemClassification.h"
#include "ContentRegistry.h"
#include "ContentRegistryIO.h"

namespace ContentRegistryIO
{

void load(ContentRegistry& reg, std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream f(resolved);
	if (!f.is_open())
		return; // file absent -- bootstrap defaults stand

	try
	{
		nlohmann::json root = nlohmann::json::parse(f);

		if (root.contains("items"))
		{
			for (auto& [key, val] : root["items"].items())
			{
				for (int i = 1; i <= static_cast<int>(ItemId::LOCKPICK); ++i)
				{
					ItemId id = static_cast<ItemId>(i);
					if (ContentRegistry::item_key(id) == key)
					{
						reg.set_tile(id, val.get<int>());
						break;
					}
				}
			}
		}

		if (root.contains("monsters"))
		{
			for (auto& [key, val] : root["monsters"].items())
			{
				for (int i = 0; i <= static_cast<int>(MonsterId::SPIDER_WEAVER); ++i)
				{
					MonsterId id = static_cast<MonsterId>(i);
					if (ContentRegistry::monster_key(id) == key)
					{
						reg.set_tile(id, val.get<int>());
						break;
					}
				}
			}
		}
	}
	catch (...)
	{
		// Malformed JSON -- bootstrap defaults stand
	}
}

void save(const ContentRegistry& reg, std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	nlohmann::json items_obj = nlohmann::json::object();
	for (int i = 1; i <= static_cast<int>(ItemId::LOCKPICK); ++i)
	{
		ItemId id = static_cast<ItemId>(i);
		items_obj[std::string{ ContentRegistry::item_key(id) }] = reg.get_tile(id);
	}

	nlohmann::json monsters_obj = nlohmann::json::object();
	for (int i = 0; i <= static_cast<int>(MonsterId::SPIDER_WEAVER); ++i)
	{
		MonsterId id = static_cast<MonsterId>(i);
		monsters_obj[std::string{ ContentRegistry::monster_key(id) }] = reg.get_tile(id);
	}

	nlohmann::json root;
	root["items"] = items_obj;
	root["monsters"] = monsters_obj;

	std::ofstream f(resolved);
	if (!f.is_open())
		throw std::runtime_error(std::format("ContentRegistryIO::save -- cannot open '{}' for writing", resolved.string()));
	f << root.dump(4);
	if (f.fail())
		throw std::runtime_error(std::format("ContentRegistryIO::save -- write failed for '{}'", resolved.string()));
}

} // namespace ContentRegistryIO

// end of file: ContentRegistryIO.cpp
