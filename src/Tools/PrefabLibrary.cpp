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
	symbol_to_tile[sym] = tile;
	symbol_to_label[sym] = std::string(label);
	palette_order.emplace_back(sym, std::string(label));
}

void PrefabLibrary::build_structural_symbols()
{
	symbol_to_tile.clear();
	symbol_to_label.clear();
	palette_order.clear();

	// Structural markers -- no decoration sprite
	register_symbol('#', TileRef{}, "Wall");
	register_symbol('.', TileRef{}, "Floor");
	register_symbol(',', TileRef{}, "Corridor");
	register_symbol('+', TileRef{}, "Door");
	register_symbol('~', TileRef{}, "Water");
}

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
			std::string sym_str = e.value("symbol", "");

			if (sym_str.empty())
			{
				continue;
			}

			char sym = sym_str[0];
			TileRef tile{
				static_cast<TileSheet>(e.value("sheet", 0)),
				e.value("col", -1),
				e.value("row", -1)
			};
			std::string label = e.value("label", sym_str);

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

TileRef PrefabLibrary::resolve_decor(char c) const
{
	auto it = symbol_to_tile.find(c);
	return (it != symbol_to_tile.end()) ? it->second : TileRef{};
}

bool PrefabLibrary::is_decoration(char c) const
{
	return resolve_decor(c).is_valid();
}

std::string PrefabLibrary::symbol_label(char c) const
{
	auto it = symbol_to_label.find(c);
	return (it != symbol_to_label.end()) ? it->second : std::string(1, c);
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
			p.depth_min = entry.value("depth_min", 1);
			p.depth_max = entry.value("depth_max", 99);
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
		entry["depth_min"] = p.depth_min;
		entry["depth_max"] = p.depth_max;
		entry["weight"] = p.weight;
		entry["rows"] = p.rows;
		arr.push_back(entry);
	}
	j["prefabs"] = arr;

	std::ofstream out(path.data());
	out << j.dump(2);
}

void PrefabLibrary::set_symbol_tile(char sym, TileRef tile)
{
	symbol_to_tile[sym] = tile;
}

void PrefabLibrary::set_symbol_label(char sym, const std::string& label)
{
	symbol_to_label[sym] = label;
	auto find_sym = [sym](const auto& e)
	{
		return e.first == sym;
	};
	auto it = std::ranges::find_if(palette_order, find_sym);
	if (it != palette_order.end())
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

// Resolves prefab name to index.  Returns -1 if not found.
static int find_prefab_index(
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

static void stamp_room(
	const Prefab& p,
	const DungeonRoom& room,
	const std::unordered_map<char, TileRef>& symbol_to_tile,
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
			auto it = symbol_to_tile.find(sym);
			if (it == symbol_to_tile.end() || !it->second.is_valid())
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
	stamp_room(prefabs[idx], room, symbol_to_tile, editor, map);
}

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
