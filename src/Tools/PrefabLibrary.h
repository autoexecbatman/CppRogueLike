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
	int depthMin{ 1 };
	int depthMax{ 99 };
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

	void load_tile_labels(std::string_view path);

	void add_or_replace(Prefab p);
	void remove(const std::string& name);

	int count() const { return static_cast<int>(prefabs.size()); }
	const std::vector<Prefab>& all() const { return prefabs; }

	TileRef resolve_decor(char c) const;
	bool is_decoration(char c) const;
	std::string symbol_label(char c) const;
	
	// TODO: Usage of pair. Prefer returning a struct with named fields for clarity.
	// Ordered palette of all known symbols (for Room Editor UI).
	const std::vector<std::pair<char, std::string>>& ordered_palette() const { return paletteOrder; }

	void set_symbol_tile(char sym, TileRef tile);
	void set_symbol_label(char sym, const std::string& label);
	void apply_to_room(
		const DungeonRoom& room,
		DecorEditor& editor,
		const Map& map) const;
	void apply_to_rooms(
		const std::vector<DungeonRoom>& rooms,
		DecorEditor& editor,
		const Map& map) const;

private:
	std::vector<Prefab> prefabs;
	std::unordered_map<char, TileRef> symbolToTile; // char -> tile (invalid = structural)
	std::unordered_map<char, std::string> symbolToLabel;
	std::vector<std::pair<char, std::string>> paletteOrder; // stable display order

	void build_structural_symbols();
	void register_symbol(char sym, TileRef tile, std::string_view label);
};
