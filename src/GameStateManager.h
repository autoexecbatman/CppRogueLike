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

// Writes every trap on the level into j["traps"], and reads them back. Traps were
// absent from the save entirely: a level reloaded in a fresh process had none, and one
// reloaded mid-session kept the abandoned level's traps at their old coordinates.
// load_traps clears the container first for that reason.
//
// Example, a level holding a hidden pit and a dart a thief has already tried:
//   save_traps(traps, level);            // level["traps"] is an array of two
//   load_traps(level, traps, tiles);     // both back, states and attempts intact
void save_traps(const std::vector<std::unique_ptr<Trap>>& traps, json& output);
void load_traps(const json& source, std::vector<std::unique_ptr<Trap>>& traps, const TileConfig& tileConfig);

// Reads every creature in j["creatures"] into the level, replacing whatever it held.
//
// Example, a level abandoned with one goblin, loaded from a save of two:
//   load_creatures(saved, creatures);   // creatures.size() -> 2, the saved two
void load_creatures(const json& source, std::vector<std::unique_ptr<Creature>>& creatures);

// Reads every room in j["rooms"] into the level, replacing whatever it held.
//
// Example, a level of five rooms abandoned for a save of three:
//   load_rooms(saved, rooms);   // rooms.size() -> 3
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
