// file: PrefabLibrary.cpp
#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "../Map/DungeonRoom.h"
#include "../Map/Map.h"
#include "../Renderer/Renderer.h"
#include "DecorEditor.h"
#include "PrefabLibrary.h"

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Symbol map
// ---------------------------------------------------------------------------

void PrefabLibrary::register_symbol(char sym, TileRef tile, std::string_view label)
{
	symbolToTile[sym] = tile;
	symbolToLabel[sym] = std::string(label);
	paletteOrder.emplace_back(sym, std::string(label));
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

	std::ifstream in(path.data());
	if (!in.is_open())
		return;

	json j;
	try
	{
		in >> j;

		if (!j.contains("palette"))
		{
			return;
		}

		for (const auto& e : j["palette"])
		{
			std::string symbolString = e.value("symbol", "");

			if (symbolString.empty())
			{
				continue;
			}

			char sym = symbolString[0];
			TileRef tile{
				static_cast<TileSheet>(e.value("sheet", 0)),
				e.value("col", -1),
				e.value("row", -1)
			};
			std::string label = e.value("label", symbolString);

			register_symbol(sym, tile, label);
		}
	}
	catch (const json::exception& e)
	{
		std::clog << std::format("[PrefabLibrary] tile_config.json error: {}\n", e.what());
	}
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

// Returns decoration TileRef for symbol; invalid TileRef for structural symbols (wall/floor/corridor/door/water).
TileRef PrefabLibrary::resolve_decor(char c) const
{
	auto it = symbolToTile.find(c);
	return (it != symbolToTile.end()) ? it->second : TileRef{};
}

// Returns true if symbol maps to a decoration (has a sprite to overlay).
bool PrefabLibrary::is_decoration(char c) const
{
	return resolve_decor(c).is_valid();
}

// Human-readable label for a symbol.
std::string PrefabLibrary::symbol_label(char c) const
{
	auto it = symbolToLabel.find(c);
	return (it != symbolToLabel.end()) ? it->second : std::string(1, c);
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PrefabLibrary::load(std::string_view path)
{
	std::ifstream in(path.data());
	
	if (!in.is_open())
	{
		return;
	}

	json j;
	try
	{
		in >> j;

		if (!j.contains("prefabs"))
		{
			return;
		}

		prefabs.clear();
		for (const auto& entry : j["prefabs"])
		{
			Prefab p;
			p.name = entry.value("name", "unnamed");
			p.depthMin = entry.value("depth_min", 1);
			p.depthMax = entry.value("depth_max", 99);
			p.weight = entry.value("weight", 1.0f);

			if (entry.contains("rows"))
			{
				for (const auto& row : entry["rows"])
				{
					p.rows.push_back(row.get<std::string>());
				}
			}

			if (!p.rows.empty())
			{
				prefabs.push_back(std::move(p));
			}
		}
	}
	catch (const json::exception& e)
	{
		std::clog << std::format("[PrefabLibrary] prefabs.json error, clearing: {}\n", e.what());
		prefabs.clear();
	}
}

void PrefabLibrary::save(std::string_view path) const
{
	json j;
	json arr = json::array();
	for (const auto& p : prefabs)
	{
		json entry;
		entry["name"] = p.name;
		entry["depth_min"] = p.depthMin;
		entry["depth_max"] = p.depthMax;
		entry["weight"] = p.weight;
		entry["rows"] = p.rows;
		arr.push_back(entry);
	}
	j["prefabs"] = arr;

	std::ofstream out(path.data());
	out << j.dump(2);
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
	auto find_sym = [sym](const auto& e)
	{
		return e.first == sym;
	};
	auto it = std::ranges::find_if(paletteOrder, find_sym);
	if (it != paletteOrder.end())
	{
		it->second = label;
	}
}

void PrefabLibrary::add_or_replace(Prefab p)
{
	auto find_name = [&p](const Prefab& existing)
	{
		return existing.name == p.name;
	};
	auto it = std::ranges::find_if(prefabs, find_name);
	if (it != prefabs.end())
	{
		*it = std::move(p);
	}
	else
	{
		prefabs.push_back(std::move(p));
	}
}

void PrefabLibrary::remove(const std::string& name)
{
	std::erase_if(prefabs, [&](const Prefab& p)
		{ return p.name == name; });
}

// ---------------------------------------------------------------------------
// Apply to rooms
// ---------------------------------------------------------------------------

namespace
{
// Resolves prefab name to index.  Returns -1 if not found.
int find_prefab_index(
	const std::vector<Prefab>& prefabs,
	const std::string& name)
{
	for (int i = 0; i < static_cast<int>(prefabs.size()); ++i)
	{
		if (prefabs[i].name == name)
		{
			return i;
		}
	}
	return -1;
}

void stamp_room(
	const Prefab& p,
	const DungeonRoom& room,
	const std::unordered_map<char, TileRef>& symbolToTile,
	DecorEditor& editor,
	const Map& map)
{
	// Prefab origin [0,0] = top-left wall corner of the room.
	// Room was sized to (p.width()-2) x (p.height()-2) so the '#' border
	// aligns exactly with the room wall ring -- no centering offset needed.
	const int base_x = room.left_wall();
	const int base_y = room.top_wall();

	for (size_t r = 0; r < p.rows.size(); ++r)
	{
		const std::string& row_str = p.rows[r];
		for (size_t c = 0; c < row_str.size(); ++c)
		{
			char sym = row_str[c];
			auto it = symbolToTile.find(sym);
			if (it == symbolToTile.end() || !it->second.is_valid())
			{
				continue;
			}
			const TileRef tile = it->second;

			const int world_x = base_x + static_cast<int>(c);
			const int world_y = base_y + static_cast<int>(r);

			if (world_x < room.col || world_x > room.col_end())
			{
				continue;
			}

			if (world_y < room.row || world_y > room.row_end())
			{
				continue;
			}

			if (!map.is_in_bounds({ world_x, world_y }))
			{
				continue;
			}

			if (map.get_tile_type({ world_x, world_y }) != TileType::FLOOR)
			{
				continue;
			}

			editor.place_tile(world_x, world_y, tile);
		}
	}
}
} // namespace

// Stamps decoration tiles from one room's assigned prefab into editor overrides.
// Called from Map::create_room before spawn_water so water can see decoration positions.
void PrefabLibrary::apply_to_room(
	const DungeonRoom& room,
	DecorEditor& editor,
	const Map& map) const
{
	if (room.prefab_name.empty())
	{
		return;
	}
	int idx = find_prefab_index(prefabs, room.prefab_name);
	if (idx < 0)
	{
		return;
	}
	stamp_room(prefabs[idx], room, symbolToTile, editor, map);
}

// Batch version used as a fallback; prefer apply_to_room called per room.
void PrefabLibrary::apply_to_rooms(
	const std::vector<DungeonRoom>& rooms,
	DecorEditor& editor,
	const Map& map) const
{
	for (const DungeonRoom& room : rooms)
	{
		apply_to_room(room, editor, map);
	}
}
