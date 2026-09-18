#pragma once

// Forward declarations
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
