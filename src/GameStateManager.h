#pragma once

#include <memory>
#include <vector>

#include "Persistent.h" // json

// Forward declarations
class Trap;
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
void save_traps(const std::vector<std::unique_ptr<Trap>>& traps, json& j);
void load_traps(const json& j, std::vector<std::unique_ptr<Trap>>& traps, const TileConfig& tileConfig);

// - Handles game state persistence and level management
class GameStateManager
{
public:
	// High-level game state operations
	bool load_all(GameContext& ctx);
	void init_new_game(GameContext& ctx);

	// Save/Load operations
	void save_game(GameContext& ctx);
	bool load_game(GameContext& ctx);

	// File operations
	static bool save_file_exists();
	static bool delete_save_file();
};
