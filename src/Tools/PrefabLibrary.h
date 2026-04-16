#pragma once
// file: PrefabLibrary.h
//
// Stores room decoration templates ("prefabs").
// Each prefab is an ASCII grid of symbols.  A symbol map translates characters
// to tile types (structural) and decoration sprite IDs (visual overlay).
//
// Prefabs are stamped onto BSP-generated rooms at dungeon startup.
// Only the decoration layer is applied for now; base tile geometry comes from BSP.
//
// Coordinate convention:
//   Prefab (col=0, row=0) = top-left wall corner of room (world_x = begin.x - 1, world_y = begin.y - 1)
//   Prefab includes the wall border ('#' characters on all four edges).

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../Renderer/Renderer.h"

class DecorEditor;
class Map;
struct DungeonRoom;

// ---------------------------------------------------------------------------

struct Prefab
{
	std::string name;
	int depth_min{ 1 };
	int depth_max{ 99 };
	float weight{ 1.0f };
	std::vector<std::string> rows; // ASCII rows; all rows must be the same length

	int width() const { return rows.empty() ? 0 : static_cast<int>(rows[0].size()); }
	int height() const { return static_cast<int>(rows.size()); }
};

// ---------------------------------------------------------------------------

class PrefabLibrary
{
public:
	void load(std::string_view path);
	void save(std::string_view path) const;

	// Rebuild the symbol map from tile_labels.json.
	// Call this before load() so prefab symbols resolve correctly.
	void load_tile_labels(std::string_view path);

	void add_or_replace(Prefab p);
	void remove(const std::string& name);

	int count() const { return static_cast<int>(prefabs.size()); }
	const std::vector<Prefab>& all() const { return prefabs; }

	// Returns decoration TileRef for symbol; invalid TileRef for structural symbols (wall/floor/corridor/door/water).
	TileRef resolve_decor(char c) const;

	// Returns true if symbol maps to a decoration (has a sprite to overlay).
	bool is_decoration(char c) const;

	// Human-readable label for a symbol.
	std::string symbol_label(char c) const;

	// Ordered palette of all known symbols (for Room Editor UI).
	const std::vector<std::pair<char, std::string>>& ordered_palette() const { return palette_order; }

	// Overwrite the tile sprite assigned to a symbol in the map.
	void set_symbol_tile(char sym, TileRef tile);

	// Overwrite the display label for a symbol.
	void set_symbol_label(char sym, const std::string& label);

	// Stamps decoration tiles from one room's assigned prefab into editor overrides.
	// Called from Map::create_room before spawn_water so water can see decoration positions.
	void apply_to_room(
		const DungeonRoom& room,
		DecorEditor& editor,
		const Map& map) const;

	// Batch version used as a fallback; prefer apply_to_room called per room.
	void apply_to_rooms(
		const std::vector<DungeonRoom>& rooms,
		DecorEditor& editor,
		const Map& map) const;

private:
	std::vector<Prefab> prefabs;
	std::unordered_map<char, TileRef> symbol_to_tile; // char -> tile (invalid = structural)
	std::unordered_map<char, std::string> symbol_to_label;
	std::vector<std::pair<char, std::string>> palette_order; // stable display order

	void build_structural_symbols();
	void register_symbol(char sym, TileRef tile, std::string_view label);
};
