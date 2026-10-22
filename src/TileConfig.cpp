// file: TileConfig.cpp
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "Paths.h"
#include "Renderer.h"
#include "TileConfig.h"
#include "TileRefJson.h"

namespace
{
// Tile types are authored by name so the file reads as the game does, and a
// typo fails to load rather than silently selecting the wrong tile.
TileType parse_tile_type(std::string_view name)
{
	if (name == "floor")
	{
		return TileType::FLOOR;
	}
	if (name == "wall")
	{
		return TileType::WALL;
	}
	if (name == "water")
	{
		return TileType::WATER;
	}
	if (name == "closed_door")
	{
		return TileType::CLOSED_DOOR;
	}
	if (name == "open_door")
	{
		return TileType::OPEN_DOOR;
	}
	if (name == "corridor")
	{
		return TileType::CORRIDOR;
	}

	throw std::runtime_error(std::format("TileConfig: unknown tile type '{}'", name));
}

// The state that lets a creature cross a tile it would otherwise be stopped by.
ActorState parse_bypass_state(std::string_view name)
{
	if (name == "can_swim")
	{
		return ActorState::CAN_SWIM;
	}

	throw std::runtime_error(std::format("TileConfig: unknown bypass state '{}'", name));
}

TileColor parse_color(const json& value)
{
	return TileColor{
		value.at("r").get<int>(),
		value.at("g").get<int>(),
		value.at("b").get<int>(),
		value.at("a").get<int>(),
	};
}

TileDefinition parse_tile_definition(const json& value)
{
	TileDefinition definition{};

	definition.displayName = value.at("display_name").get<std::string>();
	definition.blocksMovement = value.at("blocks_movement").get<bool>();

	// Optional: most tiles have nothing that crosses them and nothing to say.
	if (value.contains("bypass_state"))
	{
		definition.hasBypassState = true;
		definition.bypassState = parse_bypass_state(value.at("bypass_state").get<std::string>());
	}
	if (value.contains("entry_message"))
	{
		definition.entryMessage = value.at("entry_message").get<std::string>();
	}
	if (value.contains("blocked_message"))
	{
		definition.blockedMessage = value.at("blocked_message").get<std::string>();
	}

	definition.minimapVisible = parse_color(value.at("minimap_visible"));
	definition.minimapRemembered = parse_color(value.at("minimap_remembered"));

	return definition;
}
} // namespace

using json = nlohmann::json;

TileRef TileConfig::get(std::string_view key) const
{
	if (!tiles.contains(std::string(key)))
	{
		fprintf(stderr, "ERROR: TileConfig::get -- unknown key '%s'\n", std::string(key).c_str());
		fflush(stderr);
		throw std::runtime_error(std::format("TileConfig::get -- unknown key '{}'", key));
	}
	return tiles.at(std::string(key));
}

AutotileGroup TileConfig::get_autotile(std::string_view key) const
{
	if (!autotileGroups.contains(std::string(key)))
	{
		throw std::runtime_error(std::format("TileConfig::get_autotile -- unknown key '{}'", key));
	}
	return autotileGroups.at(std::string(key));
}

WallAutotileGroup TileConfig::get_wall_autotile(std::string_view key) const
{
	if (!wallAutotileGroups.contains(std::string(key)))
	{
		throw std::runtime_error(std::format("TileConfig::get_wall_autotile -- unknown key '{}'", key));
	}
	return wallAutotileGroups.at(std::string(key));
}

const TileDefinition& TileConfig::get_tile_definition(TileType tileType) const
{
	const auto found = tileDefinitions.find(tileType);
	if (found == tileDefinitions.end())
	{
		throw std::runtime_error(std::format(
			"TileConfig: no definition for tile type {} -- add it to tile_config.json",
			static_cast<int>(tileType)));
	}

	return found->second;
}

void TileConfig::load(std::string_view path)
{
	auto resolved = Paths::resolve(path);
	std::ifstream configFile(resolved);

	if (!configFile.is_open())
	{
		throw std::runtime_error(std::format("TileConfig::load -- cannot open '{}' -- tile config JSON is required", resolved.string()));
	}

	const json root = json::parse(configFile);

	// All four sections are required: a missing one throws here, naming itself.
	for (const auto& [key, record] : root.at("tiles").items())
	{
		tiles[key] = TileRefJson::parse_tile(record);
	}

	for (const auto& [key, record] : root.at("autotile_groups").items())
	{
		const TileSheet sheet = static_cast<TileSheet>(record.at("sheet").get<int>());
		autotileGroups[key] = AutotileGroup{
			sheet,
			record.at("origin_col").get<int>(),
			record.at("origin_row").get<int>(),
		};
	}

	for (const auto& [key, record] : root.at("wall_autotile_groups").items())
	{
		const TileSheet sheet = static_cast<TileSheet>(record.at("sheet").get<int>());
		wallAutotileGroups[key] = WallAutotileGroup{
			sheet,
			record.at("origin_col").get<int>(),
			record.at("origin_row").get<int>(),
		};
	}

	for (const auto& [key, record] : root.at("tile_types").items())
	{
		tileDefinitions[parse_tile_type(key)] = parse_tile_definition(record);
	}
}

// end of file: TileConfig.cpp
