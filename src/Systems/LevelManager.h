#pragma once

#include <string>
#include <nlohmann/json.hpp>

// Forward declarations
class Map;
class Player;
class MessageSystem;
struct GameContext;

// - Handles dungeon level progression and level-specific state
class LevelManager
{
public:
    LevelManager() = default;
    ~LevelManager() = default;

    // Level state
    int get_dungeon_level() const noexcept { return dungeon_level; }
    int get_shopkeepers_count() const noexcept { return shopkeepers_on_current_level; }
    
    // Level management
    void advance_to_next_level(Map& map, Player& player, MessageSystem& message_system, GameContext& ctx);
    void reset_to_first_level();
    
    // Shopkeeper management
    void increment_shopkeeper_count() { shopkeepers_on_current_level++; }
    bool can_spawn_shopkeeper(int max_shopkeepers = 3) const noexcept;
    
    // Save/Load
    void save_to_json(nlohmann::json& j) const;
    void load_from_json(const nlohmann::json& j);

private:
    int dungeon_level{ 1 };
    int shopkeepers_on_current_level{ 0 };
    
    // Helper methods
    void display_level_messages(MessageSystem& message_system) const;
    void heal_player_between_levels(Player& player) const;
};
