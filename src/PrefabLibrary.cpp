// file: PrefabLibrary.cpp
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "DecorEditor.h"
#include "DungeonRoom.h"
#include "Map.h"
#include "Palette.h"
#include "Paths.h"
#include "PrefabLibrary.h"
#include "Renderer.h"

namespace
{
// Resolves prefab name to index. Returns nullopt if not found.
std::optional<size_t> find_prefab_index(
	const std::vector<Prefab>& prefabs,
	const std::string& name)
{
	for (size_t index = 0; index < prefabs.size(); ++index)
	{
		if (prefabs[index].name == name)
		{
			return index;
		}
	}
	return std::nullopt;
}

void stamp_room(
	const Prefab& prefab,
	const DungeonRoom& room,
	const Palette& palette,
	DecorEditor& editor,
	Map& map)
{
	// Prefab [0,0] is the room's top-left wall corner; the room was sized so the '#' border is its wall ring.
	const int baseX = room.left_wall();
	const int baseY = room.top_wall();

	for (size_t row = 0; row < prefab.rows.size(); ++row)
	{
		const std::string& rowStr = prefab.rows[row];
		for (size_t col = 0; col < rowStr.size(); ++col)
		{
			char sym = rowStr[col];

			const int worldX = baseX + static_cast<int>(col);
			const int worldY = baseY + static_cast<int>(row);

			// Clamp to floor area - border '#' characters sit in the wall ring
			// and must never be processed as interior shape commands.
			if (worldX < room.col || worldX > room.col_end())
			{
				continue;
			}
			if (worldY < room.row || worldY > room.row_end())
			{
				continue;
			}
			if (!map.is_in_bounds({ worldX, worldY }))
			{
				continue;
			}

			// Interior '#': wall this floor cell back to create room shape.
			if (sym == '#')
			{
				map.set_tile(Vector2D{ worldX, worldY }, TileType::WALL);
				continue;
			}

			const TileRef tile = palette.resolve_decor(sym);
			if (!tile.is_valid())
			{
				continue;
			}
			if (map.get_tile_type({ worldX, worldY }) != TileType::FLOOR)
			{
				continue;
			}

			editor.place_tile(Vector2D{ worldX, worldY }, tile);
		}
	}
}
} // namespace

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PrefabLibrary::load(std::string_view path)
{
	const std::filesystem::path resolved = Paths::resolve(path);
	std::ifstream prefabsFile(resolved);
	if (!prefabsFile.is_open())
	{
		throw std::runtime_error(
			std::format(
				"PrefabLibrary::load -- cannot open '{}'",
				resolved.string()));
	}

	const json prefabsDocument = json::parse(prefabsFile);

	// Read into a local, so a record that is refused leaves the library as it was.
	std::vector<Prefab> loaded;
	for (const auto& entry : prefabsDocument.at("prefabs"))
	{
		Prefab prefab;
		prefab.name = entry.at("name").get<std::string>();
		prefab.depthMin = entry.at("depth_min").get<int>();
		prefab.depthMax = entry.at("depth_max").get<int>();
		prefab.weight = entry.at("weight").get<float>();
		prefab.rows = entry.at("rows").get<std::vector<std::string>>();

		if (prefab.rows.empty())
		{
			throw std::runtime_error(
				std::format(
					"PrefabLibrary::load -- prefab '{}' has no rows",
					prefab.name));
		}
		loaded.push_back(std::move(prefab));
	}
	prefabs = std::move(loaded);
}

void PrefabLibrary::save(std::string_view path) const
{
	json prefabsDocument;
	json arr = json::array();
	for (const auto& prefab : prefabs)
	{
		json entry;
		entry["name"] = prefab.name;
		entry["depth_min"] = prefab.depthMin;
		entry["depth_max"] = prefab.depthMax;
		entry["weight"] = prefab.weight;
		entry["rows"] = prefab.rows;
		arr.push_back(entry);
	}
	prefabsDocument["prefabs"] = arr;

	// Written where load reads it, so the editor saves to the repository's data/ from any working directory.
	const std::filesystem::path resolved = Paths::resolve(path);
	std::ofstream out(resolved);
	if (!out.is_open())
	{
		throw std::runtime_error(
			std::format(
				"PrefabLibrary::save -- cannot write '{}'",
				resolved.string()));
	}
	out << prefabsDocument.dump(2);
	out.flush();
	if (!out)
	{
		throw std::runtime_error(
			std::format(
				"PrefabLibrary::save -- cannot write '{}'; the write failed",
				resolved.string()));
	}
}

void PrefabLibrary::add_or_replace(Prefab prefab)
{
	auto has_same_name = [&prefab](const Prefab& existing)
	{
		return existing.name == prefab.name;
	};
	auto matches = prefabs | std::views::filter(has_same_name);
	if (matches.empty())
	{
		prefabs.push_back(std::move(prefab));
	}
	else
	{
		matches.front() = std::move(prefab);
	}
}

void PrefabLibrary::remove(const std::string& name)
{
	std::erase_if(
		prefabs,
		[&](const Prefab& prefab) { return prefab.name == name; });
}

// ---------------------------------------------------------------------------
// Apply to rooms
// ---------------------------------------------------------------------------

// Stamps decoration tiles from one room's assigned prefab into editor overrides.
// Called from Map::create_room before spawn_water so water can see decoration positions.
void PrefabLibrary::apply_to_room(
	const Palette& palette,
	const DungeonRoom& room,
	DecorEditor& editor,
	Map& map) const
{
	if (room.prefabName.empty())
	{
		return;
	}
	const std::optional<size_t> prefabIndex =
		find_prefab_index(prefabs, room.prefabName);
	if (!prefabIndex)
	{
		return;
	}
	stamp_room(prefabs[*prefabIndex], room, palette, editor, map);
}

// Batch version used as a fallback; prefer apply_to_room called per room.
void PrefabLibrary::apply_to_rooms(
	const Palette& palette,
	const std::vector<DungeonRoom>& rooms,
	DecorEditor& editor,
	Map& map) const
{
	for (const DungeonRoom& room : rooms)
	{
		apply_to_room(palette, room, editor, map);
	}
}
