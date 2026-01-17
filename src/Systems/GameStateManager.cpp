// GameStateManager.cpp - Handles game state persistence and level management
#include <fstream>
#include <format>

#include <nlohmann/json.hpp>

#include "GameStateManager.h"
#include "../Map/Map.h"
#include "../ActorTypes/Player.h"
#include "../Actor/InventoryOperations.h"
#include "../Actor/InventoryData.h"
#include "../Actor/Actor.h"
#include "../Gui/Gui.h"
#include "../Systems/HungerSystem.h"
#include "../Systems/LevelManager.h"
#include "../Factories/ItemCreator.h"
#include "../Utils/Vector2D.h"
#include "../Core/GameContext.h"
#include "../Systems/DataManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/MenuManager.h"

using json = nlohmann::json;
using namespace InventoryOperations;

void GameStateManager::init_new_game(GameContext& ctx)
{
    ctx.data_manager->load_all_data(*ctx.message_system);
    ctx.map->init(true, ctx);
    ctx.player->roll_new_character(ctx);
    *ctx.game_status = GameStatus::STARTUP;
    ctx.message_system->log("New game initialized");
}

bool GameStateManager::load_all(GameContext& ctx)
{
    ctx.menu_manager->set_game_initialized(true);
    ctx.data_manager->load_all_data(*ctx.message_system);

    if (!load_game(*ctx.map, *ctx.rooms, *ctx.player, *ctx.stairs, *ctx.creatures,
                   *ctx.inventory_data, *ctx.gui, *ctx.hunger_system,
                   *ctx.level_manager, *ctx.time, ctx))
    {
        ctx.message_system->log("Error: Could not open save file.");
        return false;
    }

    if (ctx.isLoadedGame) *ctx.isLoadedGame = true;
    *ctx.game_status = GameStatus::STARTUP;
    ctx.message_system->log("GameStatus set to STARTUP after loading for FOV computation");
    return true;
}

void GameStateManager::save_game(
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
)
{
    std::ofstream file(SAVE_FILE_NAME);
    if (file.is_open())
    {
        json j;

        // Save the map
        map.save(j);

        // Save rooms
        save_rooms_to_json(rooms, j);

        // Save the player
        json playerJson;
        player.save(playerJson);
        j["player"] = playerJson;

        // Save the stairs
        json stairsJson;
        stairs.save(stairsJson);
        j["stairs"] = stairsJson;

        // Save creatures
        save_creatures_to_json(creatures, j);

        // Save floor items inventory
        save_inventory(inventory_data, j);

        // Save the message log
        json guiJson;
        gui.save(guiJson);
        j["gui"] = guiJson;
        
        // Save the hunger system
        json hungerJson;
        hunger_system.save(hungerJson);
        j["hunger_system"] = hungerJson;
        
        // Save the level manager
        level_manager.save_to_json(j);
        
        // Save game time
        j["time"] = game_time;

        // Write the JSON data to the file
        file << j.dump(4); // Pretty print with an indentation of 4 spaces
        file.close();
    }
    else
    {
        throw std::runtime_error("Error occurred while saving the game.");
    }
}

bool GameStateManager::load_game(
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
)
{
    std::ifstream file(SAVE_FILE_NAME);
    if (!file.is_open())
    {
        return false; // File doesn't exist or can't be opened
    }

    json j;
    file >> j;

    // Load the map
    map.load(j);

    // Load the rooms
    load_rooms_from_json(j, rooms);

    // Load the player
    if (j.contains("player"))
    {
        player.load(j["player"]);
    }

    // Load the stairs
    if (j.contains("stairs"))
    {
        stairs.load(j["stairs"]);
    }

    // Load creatures
    load_creatures_from_json(j, creatures);

    // Load floor items inventory
    load_inventory(inventory_data, j);

    // Load the message log
    if (j.contains("gui"))
    {
        gui.load(j["gui"]);
    }
    
    // Load the hunger system
    if (j.contains("hunger_system"))
    {
        hunger_system.load(ctx, j["hunger_system"]);
    }
    
    // Load the level manager
    level_manager.load_from_json(j);
    
    // Load game time
    if (j.contains("time"))
    {
        game_time = j["time"];
    }

    return true; // Successfully loaded
}



bool GameStateManager::save_file_exists() noexcept
{
    std::ifstream file(SAVE_FILE_NAME);
    return file.good();
}

void GameStateManager::save_rooms_to_json(const std::vector<Vector2D>& rooms, nlohmann::json& j) const
{
    j["rooms"] = json::array();
    for (const auto& room : rooms)
    {
        j["rooms"].push_back({ {"x", room.x}, {"y", room.y} });
    }
}

void GameStateManager::load_rooms_from_json(const nlohmann::json& j, std::vector<Vector2D>& rooms) const
{
    if (j.contains("rooms") && j["rooms"].is_array())
    {
        for (const auto& roomData : j["rooms"])
        {
            rooms.push_back(Vector2D{ roomData["y"], roomData["x"] });
        }
    }
}

void GameStateManager::save_creatures_to_json(const std::vector<std::unique_ptr<Creature>>& creatures, nlohmann::json& j) const
{
    j["creatures"] = json::array();
    for (const auto& creature : creatures)
    {
        if (creature)
        {
            json creatureJson;
            creature->save(creatureJson);
            j["creatures"].push_back(creatureJson);
        }
    }
}

void GameStateManager::load_creatures_from_json(const nlohmann::json& j, std::vector<std::unique_ptr<Creature>>& creatures) const
{
    if (j.contains("creatures") && j["creatures"].is_array())
    {
        for (const auto& creatureData : j["creatures"])
        {
            auto creature = std::make_unique<Creature>(Vector2D{ 0, 0 }, ActorData{ ' ', "Unnamed", WHITE_BLACK_PAIR });
            creature->load(creatureData);
            
            // CRITICAL FIX: Ensure creature inventory items have correct values
            for (const auto& item : creature->inventory_data.items)
            {
                if (item)
                {
                    ItemCreator::ensure_correct_value(*item);
                }
            }

            creatures.push_back(std::move(creature));
        }
    }
}

