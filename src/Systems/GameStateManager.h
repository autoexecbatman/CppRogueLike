#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string_view>
#include <vector>

// Forward declarations
class Map;
class Player;
class Stairs;
class Creature;
class Item;
class Gui;
class HungerSystem;
class LevelManager;
struct InventoryData;
struct Vector2D;
struct DungeonRoom;
struct GameContext;

// - Handles game state persistence and level management
class GameStateManager
{
public:
	GameStateManager() = default;
	~GameStateManager() = default;

	// High-level game state operations
	bool load_all(GameContext& ctx);
	void init_new_game(GameContext& ctx);

	// Save/Load operations
	void save_game(GameContext& ctx);
	bool load_game(GameContext& ctx);

	// File operations
	static bool save_file_exists();
	static bool delete_save_file();

private:
	// Helper methods for JSON operations
	void save_rooms_to_json(const std::vector<DungeonRoom>& rooms, nlohmann::json& j) const;
	void load_rooms_from_json(const nlohmann::json& j, std::vector<DungeonRoom>& rooms) const;

	void save_creatures_to_json(const std::vector<std::unique_ptr<Creature>>& creatures, nlohmann::json& j) const;
	void load_creatures_from_json(const nlohmann::json& j, std::vector<std::unique_ptr<Creature>>& creatures) const;
};
