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
#include <vector>

#include "Palette.h"
#include "Renderer.h"

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
private:
	std::vector<Prefab> prefabs;
	std::unordered_map<char, TileRef> symbolToTile; // char -> tile (invalid = structural)
	std::unordered_map<char, std::string> symbolToLabel;
	std::vector<SymbolRow> paletteOrder; // stable display order

	void build_structural_symbols();
	void register_symbol(char sym, TileRef tile, std::string_view label);

public:
	// Replaces the prefabs with the records in a prefabs.json, the path resolved against the
	// repository root. Throws when the file does not open or parse, lacks "prefabs", or a
	// record lacks name, depth_min, depth_max, weight or rows, or has no rows; the prefabs
	// already loaded are then kept.
	//
	// Example:
	//   library.load(Paths::PREFABS);              // -> one prefab per record in the file
	//   library.load("data/no_such_file.json");    // throws "PrefabLibrary::load -- cannot open '...'"
	void load(std::string_view path);
	// Writes every prefab to a prefabs.json at the path resolved against the repository root.
	// Throws, naming the path, when the file cannot be opened for writing or the write fails.
	//
	// Example:
	//   library.save(Paths::PREFABS);   // -> data/prefabs.json rewritten
	//   library.save("data");           // throws "PrefabLibrary::save -- cannot write '...data'"
	void save(std::string_view path) const;

	// Rebuilds the symbol map from tile_config.json's palette: the five structural symbols, then
	// every entry with a symbol. An entry whose symbol is empty is skipped. Throws when the file
	// does not open or parse, lacks "palette", or an entry lacks symbol or label.
	//
	// Example:
	//   library.load_tile_labels(Paths::TILE_CONFIG);   // -> the 5 structural symbols, then each entry with a symbol
	void load_tile_labels(std::string_view path);

	void add_or_replace(Prefab prefab);
	void remove(const std::string& name);

	int count() const { return static_cast<int>(prefabs.size()); }

	const std::vector<Prefab>& all() const { return prefabs; }

	TileRef resolve_decor(char symbol) const;
	bool is_decoration(char symbol) const;
	std::string symbol_label(char symbol) const;

	// Ordered palette of all known symbols (for Room Editor UI).
	const std::vector<SymbolRow>& ordered_palette() const { return paletteOrder; }

	void set_symbol_tile(char sym, TileRef tile);
	void set_symbol_label(char sym, const std::string& label);
	void apply_to_room(
		const DungeonRoom& room,
		DecorEditor& editor,
		Map& map) const;
	void apply_to_rooms(
		const std::vector<DungeonRoom>& rooms,
		DecorEditor& editor,
		Map& map) const;
};
