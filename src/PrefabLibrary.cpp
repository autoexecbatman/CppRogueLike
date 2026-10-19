// file: PrefabLibrary.cpp
#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "DecorEditor.h"
#include "DungeonRoom.h"
#include "Map.h"
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
	const std::unordered_map<char, TileRef>& symbolToTile,
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

			if (!symbolToTile.contains(sym))
			{
				continue;
			}
			const TileRef tile = symbolToTile.at(sym);
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
// Symbol map
// ---------------------------------------------------------------------------

void PrefabLibrary::register_symbol(char sym, TileRef tile, std::string_view label)
{
	symbolToTile[sym] = tile;
	symbolToLabel[sym] = std::string(label);
	paletteOrder.push_back(PaletteEntry{ sym, std::string(label) });
}

void PrefabLibrary::build_structural_symbols()
{
	symbolToTile.clear();
	symbolToLabel.clear();
	paletteOrder.clear();

	// Structural markers -- no decoration sprite
	register_symbol('#', TileRef{}, "Wall");
	register_symbol('.', TileRef{}, "Floor");
	register_symbol(',', TileRef{}, "Corridor");
	register_symbol('+', TileRef{}, "Door");
	register_symbol('~', TileRef{}, "Water");
}

// Rebuild the symbol map from tile_labels.json.
// Call this before load() so prefab symbols resolve correctly.
void PrefabLibrary::load_tile_labels(std::string_view path)
{
	build_structural_symbols();

	// Resolved like every other loader: a bare relative path opens only from the repository root.
	std::ifstream tileConfigFile(Paths::resolve(path));
	if (!tileConfigFile.is_open())
	{
		return;
	}

	json tileConfigDocument;
	try
	{
		tileConfigFile >> tileConfigDocument;

		if (!tileConfigDocument.contains("palette"))
		{
			return;
		}

		for (const auto& paletteEntry : tileConfigDocument["palette"])
		{
			std::string symbolString = paletteEntry.value("symbol", "");

			if (symbolString.empty())
			{
				continue;
			}

			char sym = symbolString[0];
			TileRef tile{
				static_cast<TileSheet>(paletteEntry.value("sheet", 0)),
				paletteEntry.value("col", -1),
				paletteEntry.value("row", -1)
			};
			std::string label = paletteEntry.value("label", symbolString);

			register_symbol(sym, tile, label);
		}
	}
	catch (const json::exception& error)
	{
		std::clog << std::format("[PrefabLibrary] tile_config.json error: {}\n", error.what());
	}
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

// Returns decoration TileRef for symbol; invalid TileRef for structural symbols (wall/floor/corridor/door/water).
TileRef PrefabLibrary::resolve_decor(char symbol) const
{
	return symbolToTile.contains(symbol) ? symbolToTile.at(symbol) : TileRef{};
}

// Returns true if symbol maps to a decoration (has a sprite to overlay).
bool PrefabLibrary::is_decoration(char symbol) const
{
	return resolve_decor(symbol).is_valid();
}

// Human-readable label for a symbol.
std::string PrefabLibrary::symbol_label(char symbol) const
{
	return symbolToLabel.contains(symbol) ? symbolToLabel.at(symbol) : std::string(1, symbol);
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PrefabLibrary::load(std::string_view path)
{
	std::ifstream prefabsFile(Paths::resolve(path));

	if (!prefabsFile.is_open())
	{
		return;
	}

	json prefabsDocument;
	try
	{
		prefabsFile >> prefabsDocument;

		if (!prefabsDocument.contains("prefabs"))
		{
			return;
		}

		prefabs.clear();
		for (const auto& entry : prefabsDocument["prefabs"])
		{
			Prefab prefab;
			prefab.name = entry.value("name", "unnamed");
			prefab.depthMin = entry.value("depth_min", 1);
			prefab.depthMax = entry.value("depth_max", 99);
			prefab.weight = entry.value("weight", 1.0f);

			if (entry.contains("rows"))
			{
				for (const auto& row : entry["rows"])
				{
					prefab.rows.push_back(row.get<std::string>());
				}
			}

			if (!prefab.rows.empty())
			{
				prefabs.push_back(std::move(prefab));
			}
		}
	}
	catch (const json::exception& error)
	{
		std::clog << std::format("[PrefabLibrary] prefabs.json error, clearing: {}\n", error.what());
		prefabs.clear();
	}
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

	std::ofstream out(path.data());
	out << prefabsDocument.dump(2);
}

// Overwrite the tile sprite assigned to a symbol in the map.
void PrefabLibrary::set_symbol_tile(char sym, TileRef tile)
{
	symbolToTile[sym] = tile;
}

// Overwrite the display label for a symbol.
void PrefabLibrary::set_symbol_label(char sym, const std::string& label)
{
	symbolToLabel[sym] = label;
	auto matches_symbol = [sym](const PaletteEntry& paletteEntry)
	{
		return paletteEntry.symbol == sym;
	};
	auto matches = paletteOrder | std::views::filter(matches_symbol);
	if (!matches.empty())
	{
		matches.front().label = label;
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
	std::erase_if(prefabs, [&](const Prefab& prefab)
		{ return prefab.name == name; });
}

// ---------------------------------------------------------------------------
// Apply to rooms
// ---------------------------------------------------------------------------

// Stamps decoration tiles from one room's assigned prefab into editor overrides.
// Called from Map::create_room before spawn_water so water can see decoration positions.
void PrefabLibrary::apply_to_room(
	const DungeonRoom& room,
	DecorEditor& editor,
	Map& map) const
{
	if (room.prefabName.empty())
	{
		return;
	}
	const std::optional<size_t> prefabIndex = find_prefab_index(prefabs, room.prefabName);
	if (!prefabIndex)
	{
		return;
	}
	stamp_room(prefabs[*prefabIndex], room, symbolToTile, editor, map);
}

// Batch version used as a fallback; prefer apply_to_room called per room.
void PrefabLibrary::apply_to_rooms(
	const std::vector<DungeonRoom>& rooms,
	DecorEditor& editor,
	Map& map) const
{
	for (const DungeonRoom& room : rooms)
	{
		apply_to_room(room, editor, map);
	}
}
