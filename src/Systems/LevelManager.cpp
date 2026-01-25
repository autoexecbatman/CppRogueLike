// LevelManager.cpp - Handles dungeon level progression and level-specific state
#include <nlohmann/json.hpp>
#include <format>

#include "LevelManager.h"
#include "../Map/Map.h"
#include "../ActorTypes/Player.h"
#include "../Systems/MessageSystem.h"
#include "../Core/GameContext.h"

void LevelManager::advance_to_next_level(GameContext& ctx)
{
    dungeon_level++;
    shopkeepers_on_current_level = 0; // Reset shopkeeper counter for new level
    
    // Display progression messages
    display_level_messages(*ctx.message_system);
    
    // Heal player between levels
    heal_player_between_levels(ctx);
    
    // Regenerate the map for new level
    ctx.map->regenerate(ctx);
}

void LevelManager::reset_to_first_level()
{
    dungeon_level = 1;
    shopkeepers_on_current_level = 0;
}

bool LevelManager::can_spawn_shopkeeper(int max_shopkeepers) const noexcept
{
    return shopkeepers_on_current_level < max_shopkeepers;
}

void LevelManager::save_to_json(nlohmann::json& j) const
{
    j["level_manager"] = {
        {"dungeonLevel", dungeon_level},
        {"shopkeepersOnCurrentLevel", shopkeepers_on_current_level}
    };
}

void LevelManager::load_from_json(const nlohmann::json& j)
{
    if (j.contains("level_manager"))
    {
        const auto& level_data = j["level_manager"];
        
        if (level_data.contains("dungeonLevel"))
        {
            dungeon_level = level_data["dungeonLevel"];
        }
        
        if (level_data.contains("shopkeepersOnCurrentLevel"))
        {
            shopkeepers_on_current_level = level_data["shopkeepersOnCurrentLevel"];
        }
    }
    // Legacy support - check old format
    else
    {
        if (j.contains("dungeonLevel"))
        {
            dungeon_level = j["dungeonLevel"];
        }
        
        if (j.contains("shopkeepersOnCurrentLevel"))
        {
            shopkeepers_on_current_level = j["shopkeepersOnCurrentLevel"];
        }
    }
}

void LevelManager::display_level_messages(MessageSystem& message_system) const
{
    message_system.message(WHITE_BLACK_PAIR, "You take a moment to rest, and recover your strength.", true);
    message_system.message(WHITE_BLACK_PAIR, "After a rare moment of peace, you descend", true);
    message_system.message(WHITE_BLACK_PAIR, "deeper into the heart of the dungeon...", true);
    message_system.message(WHITE_BLACK_PAIR, std::format("You are now on level {}", dungeon_level), true);
}

void LevelManager::heal_player_between_levels(GameContext& ctx) const
{
    if (!ctx.player || !ctx.player->destructible)
    {
        return;
    }
    
    const int healAmount = ctx.player->destructible->get_max_hp() / 2;
    const int actualHealed = ctx.player->destructible->heal(healAmount);
    
    if (actualHealed > 0)
    {
        ctx.message_system->message(GREEN_BLACK_PAIR, 
            std::format("You rest between levels and recover {} HP.", actualHealed), true);
    }
}
