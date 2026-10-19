// file: ContentRegistryIO.cpp
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "ContentRegistry.h"
#include "ContentRegistryIO.h"
#include "Paths.h"
#include "Renderer.h"

namespace ContentRegistryIO
{

void load(ContentRegistry& reg, std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream tilesFile(resolved);
	if (!tilesFile.is_open())
	{
		throw std::runtime_error(std::format(
			"ContentRegistryIO::load -- cannot open '{}' -- JSON tile data is required",
			resolved.string()));
	}

	nlohmann::json root = nlohmann::json::parse(tilesFile);

	if (root.contains("items"))
	{
		for (auto& [key, val] : root["items"].items())
		{
			TileRef tile{
				static_cast<TileSheet>(val.at("sheet").get<int>()),
				val.at("col").get<int>(),
				val.at("row").get<int>()
			};
			reg.set_tile(key, tile);
		}
	}
}

void save(const ContentRegistry& reg, std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::filesystem::create_directories(resolved.parent_path());

	auto encode_tile = [](TileRef tileRef) -> nlohmann::json
	{
		return nlohmann::json{
			{ "sheet", static_cast<int>(tileRef.sheet) },
			{ "col", tileRef.col },
			{ "row", tileRef.row }
		};
	};

	nlohmann::json items_obj = nlohmann::json::object();
	for (const auto& [key, tile] : reg.all_tiles())
	{
		items_obj[key] = encode_tile(tile);
	}

	nlohmann::json root;
	root["items"] = items_obj;

	std::ofstream tilesFile(resolved);
	if (!tilesFile.is_open())
	{
		throw std::runtime_error(std::format(
			"ContentRegistryIO::save -- cannot open '{}' for writing",
			resolved.string()));
	}
	tilesFile << root.dump(4);
	if (tilesFile.fail())
	{
		throw std::runtime_error(std::format(
			"ContentRegistryIO::save -- write failed for '{}'",
			resolved.string()));
	}
}

} // namespace ContentRegistryIO

// end of file: ContentRegistryIO.cpp
