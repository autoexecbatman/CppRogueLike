#pragma once

#include <memory>
#include <vector>
#include <string_view>
#include <nlohmann/json.hpp>

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
    void save_game(
        Map& map,
        const std::vector<Vector2D>& rooms,
        Player& player,
        Stairs& stairs,
        const std::vector<std::unique_ptr<Creature>>& creatures,
        const InventoryData& inventory_data,
        Gui& gui,
        HungerSystem& hunger_system,
        const LevelManager& level_manager,
        int game_time
    );

    bool load_game(
        Map& map,
        std::vector<Vector2D>& rooms,
        Player& player,
        Stairs& stairs,
        std::vector<std::unique_ptr<Creature>>& creatures,
        InventoryData& inventory_data,
        Gui& gui,
        HungerSystem& hunger_system,
        LevelManager& level_manager,
        int& game_time,
        GameContext& ctx
    );

    // File operations
    static bool save_file_exists() noexcept;
    static bool delete_save_file() noexcept;
    
private:
    static constexpr const char* SAVE_FILE_NAME = "game.sav";
    
    // Helper methods for JSON operations
    void save_rooms_to_json(const std::vector<Vector2D>& rooms, nlohmann::json& j) const;
    void load_rooms_from_json(const nlohmann::json& j, std::vector<Vector2D>& rooms) const;
    
    void save_creatures_to_json(const std::vector<std::unique_ptr<Creature>>& creatures, nlohmann::json& j) const;
    void load_creatures_from_json(const nlohmann::json& j, std::vector<std::unique_ptr<Creature>>& creatures) const;
    
};
