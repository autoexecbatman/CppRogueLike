// file: TileConfig.cpp
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "../Core/Paths.h"
#include "../Renderer/Renderer.h"
#include "TileConfig.h"

using json = nlohmann::json;

TileRef TileConfig::get(std::string_view key) const
{
	if (!m_tiles.contains(std::string(key)))
	{
		fprintf(stderr, "ERROR: TileConfig::get -- unknown key '%s'\n", std::string(key).c_str());
		fflush(stderr);
		throw std::runtime_error(std::format("TileConfig::get -- unknown key '{}'", key));
	}
	return m_tiles.at(std::string(key));
}

AutotileGroup TileConfig::get_autotile(std::string_view key) const
{
	if (!m_autotile_groups.contains(std::string(key)))
	{
		throw std::runtime_error(std::format("TileConfig::get_autotile -- unknown key '{}'", key));
	}
	return m_autotile_groups.at(std::string(key));
}

WallAutotileGroup TileConfig::get_wall_autotile(std::string_view key) const
{
	if (!m_wall_autotile_groups.contains(std::string(key)))
	{
		throw std::runtime_error(std::format("TileConfig::get_wall_autotile -- unknown key '{}'", key));
	}
	return m_wall_autotile_groups.at(std::string(key));
}

void TileConfig::load(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream f(resolved);

	if (!f.is_open())
	{
		throw std::runtime_error(std::format("TileConfig::load -- cannot open '{}' -- tile config JSON is required", resolved.string()));
	}

	const json root = json::parse(f);

	if (root.contains("tiles"))
	{
		for (auto& [key, val] : root["tiles"].items())
		{
			auto sheet = static_cast<TileSheet>(val["sheet"].get<int>());
			int col = val["col"].get<int>();
			int row = val["row"].get<int>();
			m_tiles[key] = TileRef{ sheet, col, row };
		}
	}

	if (root.contains("autotile_groups"))
	{
		for (auto& [key, val] : root["autotile_groups"].items())
		{
			m_autotile_groups[key] = AutotileGroup{
				static_cast<TileSheet>(val["sheet"].get<int>()),
				val["origin_col"].get<int>(),
				val["origin_row"].get<int>()
			};
		}
	}

	if (root.contains("wall_autotile_groups"))
	{
		for (auto& [key, val] : root["wall_autotile_groups"].items())
		{
			m_wall_autotile_groups[key] = WallAutotileGroup{
				static_cast<TileSheet>(val["sheet"].get<int>()),
				val["origin_col"].get<int>(),
				val["origin_row"].get<int>()
			};
		}
	}
}

// end of file: TileConfig.cpp
