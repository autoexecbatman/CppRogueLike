#pragma once
// file: PrefabLibrary.h
//
// Stores room decoration templates ("prefabs").
// Each prefab is an ASCII grid of symbols. The Palette, passed to apply_to_room, says
// which symbol places which decoration; the library itself holds no symbols.
//
// Prefabs are stamped onto BSP-generated rooms at dungeon startup.
// Only the decoration layer is applied for now; base tile geometry comes from BSP.
//
// Coordinate convention:
//   Prefab (col=0, row=0) = top-left wall corner of room (world_x = begin.x - 1, world_y = begin.y - 1)
//   Prefab includes the wall border ('#' characters on all four edges).

#include <string>
#include <string_view>
#include <vector>

class DecorEditor;
class Palette;
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

	void add_or_replace(Prefab prefab);
	void remove(const std::string& name);

	int count() const { return static_cast<int>(prefabs.size()); }

	const std::vector<Prefab>& all() const { return prefabs; }

	// Stamps the room's assigned prefab: an interior '#' walls its floor cell back, and each
	// symbol the palette resolves to a decoration places that tile on a floor cell. A room with
	// no prefab, or one naming a prefab the library lacks, is left as it is.
	//
	// Example:
	//   library.apply_to_room(*ctx.palette, room, *ctx.decorEditor, map);   // a 'W' in the prefab places a cobweb
	void apply_to_room(
		const Palette& palette,
		const DungeonRoom& room,
		DecorEditor& editor,
		Map& map) const;
	// apply_to_room for each room in turn.
	//
	// Example:
	//   library.apply_to_rooms(*ctx.palette, rooms, *ctx.decorEditor, map);   // every room with a prefab is stamped
	void apply_to_rooms(
		const Palette& palette,
		const std::vector<DungeonRoom>& rooms,
		DecorEditor& editor,
		Map& map) const;
};
