// file: PrefabLibrary.cpp
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "../Map/DungeonRoom.h"
#include "../Map/Map.h"
#include "../Renderer/Renderer.h"
#include "DecorEditor.h"
#include "PrefabLibrary.h"

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Symbol map
// ---------------------------------------------------------------------------

void PrefabLibrary::build_structural_symbols()
{
	symbol_to_tile.clear();
	symbol_to_label.clear();
	palette_order.clear();

	auto reg = [&](char sym, std::string label)
	{
		symbol_to_tile[sym] = TileRef{};
		symbol_to_label[sym] = label;
		palette_order.emplace_back(sym, std::move(label));
	};

	// Structural markers -- no decoration sprite (tile_id = 0)
	reg('#', "Wall");
	reg('.', "Floor");
	reg(',', "Corridor");
	reg('+', "Door");
	reg('~', "Water");
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
			return;
		for (const auto& e : j["palette"])
		{
			std::string sym_str = e.value("symbol", "");
			if (sym_str.empty())
				continue;

			char sym = sym_str[0];
			TileRef tile{
				static_cast<TileSheet>(e.value("sheet", 0)),
				e.value("col", -1),
				e.value("row", -1)
			};
			std::string label = e.value("label", sym_str);

			symbol_to_tile[sym] = tile;
			symbol_to_label[sym] = label;
			palette_order.emplace_back(sym, label);
		}
	}
	catch (...)
	{
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
		return;

	json j;
	try
	{
		in >> j;

		if (!j.contains("prefabs"))
			return;

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
					p.rows.push_back(row.get<std::string>());
			}

			if (!p.rows.empty())
				prefabs.push_back(std::move(p));
		}
	}
	catch (...)
	{
	} // Corrupt file -- keep defaults
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
	for (auto& entry : palette_order)
	{
		if (entry.first == sym)
		{
			entry.second = label;
			break;
		}
	}
}

void PrefabLibrary::add_or_replace(Prefab p)
{
	for (auto& existing : prefabs)
	{
		if (existing.name == p.name)
		{
			existing = std::move(p);
			return;
		}
	}
	prefabs.push_back(std::move(p));
}

void PrefabLibrary::remove(const std::string& name)
{
	std::erase_if(prefabs, [&](const Prefab& p)
		{ return p.name == name; });
}

// ---------------------------------------------------------------------------
// Prefab selection
// ---------------------------------------------------------------------------

int PrefabLibrary::pick_prefab(
	int total_w,
	int total_h,
	int dungeon_level,
	long hash_val) const
{
	struct Candidate
	{
		int index;
		float weight;
	};

	std::vector<Candidate> candidates;
	candidates.reserve(prefabs.size());

	for (int i = 0; i < static_cast<int>(prefabs.size()); ++i)
	{
		const Prefab& p = prefabs[i];
		if (p.depth_min > dungeon_level || p.depth_max < dungeon_level)
			continue;
		if (p.width() > total_w)
			continue;
		if (p.height() > total_h)
			continue;
		candidates.push_back({ i, p.weight });
	}

	if (candidates.empty())
		return -1;

	float total_weight = 0.0f;
	for (const auto& c : candidates)
		total_weight += c.weight;

	// Map hash to [0, total_weight) deterministically
	float selection = static_cast<float>(std::abs(hash_val) % 1000000) / 1000000.0f * total_weight;

	float accumulated = 0.0f;
	for (const auto& c : candidates)
	{
		accumulated += c.weight;
		if (selection < accumulated)
			return c.index;
	}
	return candidates.back().index;
}

// ---------------------------------------------------------------------------
// Apply to rooms
// ---------------------------------------------------------------------------

void PrefabLibrary::apply_to_rooms(
	const std::vector<DungeonRoom>& rooms,
	long seed,
	int dungeon_level,
	DecorEditor& editor,
	const Map& map) const
{
	if (prefabs.empty() || rooms.empty())
		return;

	auto room_hash = [&](int room_idx) -> long
	{
		long h = seed ^ (static_cast<long>(room_idx) * 2654435761L);
		h ^= h >> 16;
		h *= 0x45d9f3bL;
		h ^= h >> 16;
		return h;
	};

	for (int room_idx = 0; room_idx < static_cast<int>(rooms.size()); ++room_idx)
	{
		const DungeonRoom& room = rooms[static_cast<size_t>(room_idx)];

		// Total room dimensions including surrounding walls
		const int total_w = room.total_width();
		const int total_h = room.total_height();

		int idx = pick_prefab(total_w, total_h, dungeon_level, room_hash(room_idx));
		if (idx < 0)
			continue;

		const Prefab& p = prefabs[idx];

		// Center the prefab inside the room.
		// Prefab origin [0,0] aligns to the left/top wall so the '#' border
		// of an exactly-fitting prefab lines up with the room walls.
		const int base_x = room.left_wall();
		const int base_y = room.top_wall();
		const int offset_x = (total_w - p.width()) / 2;
		const int offset_y = (total_h - p.height()) / 2;

		// Stamp the decoration layer -- floor tiles only.
		for (int r = 0; r < p.height(); ++r)
		{
			const std::string& row_str = p.rows[r];
			for (int c = 0; c < static_cast<int>(row_str.size()); ++c)
			{
				char sym = row_str[c];
				TileRef tile = resolve_decor(sym);
				if (!tile.is_valid())
					continue;

				const int world_x = base_x + offset_x + c;
				const int world_y = base_y + offset_y + r;

				// Reject anything outside room floor interior (fast early-out).
				if (world_x < room.col || world_x > room.col_end())
					continue;
				if (world_y < room.row || world_y > room.row_end())
					continue;

				// Reject if the map tile is not a plain floor tile.
				// This prevents stamping on water, corridor, wall, or any
				// other special tile type that may exist within room bounds.
				if (!map.is_in_bounds({ world_x, world_y }))
					continue;
				if (map.get_tile_type({ world_x, world_y }) != TileType::FLOOR)
					continue;

				editor.place_tile(world_x, world_y, tile);
			}
		}
	}
}
