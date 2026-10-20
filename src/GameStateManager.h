#pragma once

#include <memory>
#include <vector>

#include "Persistent.h" // json

// Forward declarations
class Trap;
struct Decoration;
class TileConfig;
class Map;
class Player;
class Stairs;
class Creature;
class Item;
class Gui;
class HungerSystem;
class LevelManager;
struct FloorInventory;
struct Vector2D;
struct DungeonRoom;
struct GameContext;

// Writes every trap on the level into j["traps"], an array even when there are none.
//
// Example, a level holding a hidden pit and a dart a thief has already tried:
//   save_traps(traps, level);            // level["traps"] is an array of two
void save_traps(const std::vector<std::unique_ptr<Trap>>& traps, json& output);

// Reads every trap in j["traps"] into the level, replacing whatever it held, so a load
// during play leaves none of the abandoned level's traps behind. The list is required: a
// missing "traps" throws json::out_of_range, and one that is not an array throws
// std::runtime_error.
//
// Example:
//   load_traps(level, traps, tiles);             // both back, states and attempts intact
//   load_traps(json::object(), traps, tiles);    // throws: key 'traps' not found
void load_traps(const json& source, std::vector<std::unique_ptr<Trap>>& traps, const TileConfig& tileConfig);

// Reads every creature in j["creatures"] into the level, replacing whatever it held. The
// list is required: a missing "creatures" throws json::out_of_range, and one that is not
// an array throws std::runtime_error.
//
// Example, a level abandoned with one goblin, loaded from a save of two:
//   load_creatures(saved, creatures);            // creatures.size() -> 2, the saved two
//   load_creatures(json::object(), creatures);   // throws: key 'creatures' not found
void load_creatures(const json& source, std::vector<std::unique_ptr<Creature>>& creatures);

// Writes every room on the level into j["rooms"]: an array even when there are none, and
// every room's col, row, width, height and type.
//
// Example:
//   save_rooms(threeRooms, saved);   // saved["rooms"] is an array of three
void save_rooms(const std::vector<DungeonRoom>& rooms, json& output);

// Reads every room in j["rooms"] into the level, replacing whatever it held. Everything
// save_rooms writes is required: a missing key throws json::out_of_range naming it, and a
// "rooms" that is not an array, or a type outside RoomType, throws std::runtime_error.
//
// Example, a level of five rooms abandoned for a save of three:
//   load_rooms(saved, rooms);            // rooms.size() -> 3
//   load_rooms(json::object(), rooms);   // throws: key 'rooms' not found
void load_rooms(const json& source, std::vector<DungeonRoom>& rooms);

// Writes every decoration on the level into j["decorations"], and reads them back,
// replacing whatever the level held. Every field is saved: a decoration is plain data,
// and a barrel hit once keeps the one hit point it has left.
//
// Example, a barrel at 1 of its 2 hit points and an editor-stamped pillar:
//   save_decorations(decorations, level);   // level["decorations"] is an array of two
//   load_decorations(level, decorations);   // both back, the barrel still at 1
void save_decorations(const std::vector<std::unique_ptr<Decoration>>& decorations, json& output);
void load_decorations(const json& source, std::vector<std::unique_ptr<Decoration>>& decorations);

// - Handles game state persistence and level management
class GameStateManager
{
public:
	// High-level game state operations
	bool load_all(GameContext& ctx);
	void init_new_game(GameContext& ctx);

	// Save/Load operations
	void save_game(GameContext& ctx);

	// Restores the saved game. A load replaces every level container; each loader clears
	// what it restores, because a load can arrive from the in-game menu with another
	// level still in memory and nothing regenerates it first. Spell tiles are not saved,
	// so they are cleared. False, and nothing changed, when there is no save file.
	//
	// Example, a load during play on a level of ten creatures, from a save of three:
	//   stateManager.load_game(ctx);   // -> true; ctx.creatures->size() -> 3
	bool load_game(GameContext& ctx);

	// File operations
	static bool save_file_exists();
	static bool delete_save_file();
};
