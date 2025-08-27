// GameStateManager.h - Handles game state persistence and level management

#ifndef GAMESTATEMANAGER_H
#define GAMESTATEMANAGER_H

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
class Container;
class Gui;
class HungerSystem;
class LevelManager;
struct Vector2D;

class GameStateManager
{
public:
    GameStateManager() = default;
    ~GameStateManager() = default;

    // Save/Load operations
    void save_game(
        Map& map,
        const std::vector<Vector2D>& rooms,
        Player& player,
        Stairs& stairs,
        const std::vector<std::unique_ptr<Creature>>& creatures,
        const Container& container,
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
        Container& container,
        Gui& gui,
        HungerSystem& hunger_system,
        LevelManager& level_manager,
        int& game_time
    );



    // File operations
    static bool save_file_exists() noexcept;
    
private:
    static constexpr const char* SAVE_FILE_NAME = "game.sav";
    
    // Helper methods for JSON operations
    void save_rooms_to_json(const std::vector<Vector2D>& rooms, nlohmann::json& j) const;
    void load_rooms_from_json(const nlohmann::json& j, std::vector<Vector2D>& rooms) const;
    
    void save_creatures_to_json(const std::vector<std::unique_ptr<Creature>>& creatures, nlohmann::json& j) const;
    void load_creatures_from_json(const nlohmann::json& j, std::vector<std::unique_ptr<Creature>>& creatures) const;
    
    void save_items_to_json(const Container& container, nlohmann::json& j) const;
    void load_items_from_json(const nlohmann::json& j, Container& container) const;
};

#endif // GAMESTATEMANAGER_H
