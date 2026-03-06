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
	auto it = m_tiles.find(std::string(key));

	if (it == m_tiles.end())
	{
		throw std::runtime_error(std::format("TileConfig::get -- unknown key '{}'", key));
	}

	return it->second;
}

AutotileGroup TileConfig::get_autotile(std::string_view key) const
{
	auto it = m_autotile_groups.find(std::string(key));

	if (it == m_autotile_groups.end())
	{
		throw std::runtime_error(std::format("TileConfig::get_autotile -- unknown key '{}'", key));
	}

	return it->second;
}

WallAutotileGroup TileConfig::get_wall_autotile(std::string_view key) const
{
	auto it = m_wall_autotile_groups.find(std::string(key));

	if (it == m_wall_autotile_groups.end())
	{
		throw std::runtime_error(std::format("TileConfig::get_wall_autotile -- unknown key '{}'", key));
	}

	return it->second;
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
