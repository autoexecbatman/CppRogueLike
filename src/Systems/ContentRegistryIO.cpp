// file: ContentRegistryIO.cpp

#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "../Core/Paths.h"
#include "../Renderer/Renderer.h"
#include "ContentRegistry.h"
#include "ContentRegistryIO.h"

namespace ContentRegistryIO
{

void load(ContentRegistry& reg, std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream f(resolved);
	if (!f.is_open())
	{
		throw std::runtime_error(std::format(
			"ContentRegistryIO::load -- cannot open '{}' -- JSON tile data is required",
			resolved.string()));
	}

	nlohmann::json root = nlohmann::json::parse(f);

	if (root.contains("items"))
	{
		for (auto& [key, val] : root["items"].items())
		{
			TileRef t{
				static_cast<TileSheet>(val.at("sheet").get<int>()),
				val.at("col").get<int>(),
				val.at("row").get<int>()
			};
			reg.set_tile(key, t);
		}
	}
}

void save(const ContentRegistry& reg, std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	auto encode_tile = [](TileRef t) -> nlohmann::json
	{
		return nlohmann::json{
			{ "sheet", static_cast<int>(t.sheet) },
			{ "col",   t.col },
			{ "row",   t.row }
		};
	};

	nlohmann::json items_obj = nlohmann::json::object();
	for (const auto& [key, tile] : reg.all_tiles())
		items_obj[key] = encode_tile(tile);

	nlohmann::json root;
	root["items"] = items_obj;

	std::ofstream f(resolved);
	if (!f.is_open())
	{
		throw std::runtime_error(std::format(
			"ContentRegistryIO::save -- cannot open '{}' for writing",
			resolved.string()));
	}
	f << root.dump(4);
	if (f.fail())
	{
		throw std::runtime_error(std::format(
			"ContentRegistryIO::save -- write failed for '{}'",
			resolved.string()));
	}
}

} // namespace ContentRegistryIO

// end of file: ContentRegistryIO.cpp
