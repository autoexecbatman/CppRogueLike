#pragma once
// file: Game.h
// Header-only game container - owns all systems, provides GameContext
// C++ Core Guidelines: C.9 minimize exposure, Rule of Zero

#include <memory>
#include <deque>
#include <vector>

#include "Core/GameContext.h"
#include "Map/Map.h"
#include "ActorTypes/Player.h"
#include "Actor/InventoryData.h"
#include "Gui/Gui.h"
#include "Menu/BaseMenu.h"
#include "Random/RandomDice.h"

#include "Systems/TargetingSystem.h"
#include "Systems/HungerSystem.h"
#include "Systems/BuffSystem.h"
#include "Systems/MessageSystem.h"
#include "Systems/RenderingManager.h"
#include "Systems/InputHandler.h"
#include "Systems/GameStateManager.h"
#include "Systems/LevelManager.h"
#include "Systems/CreatureManager.h"
#include "Systems/MenuManager.h"
#include "Systems/DisplayManager.h"
#include "Systems/GameLoopCoordinator.h"
#include "Systems/DataManager.h"

class Game
{
public:
    // Rule of Zero - compiler generates correct special members
    Game() = default;

    // Single source of truth: build context from owned systems
    [[nodiscard]] GameContext context() noexcept
    {
        return GameContext{
            // Core game world
            .map = &map,
            .gui = &gui,
            .player = player.get(),

            // Core systems
            .message_system = &message_system,
            .dice = &dice,

            // Managers
            .creature_manager = &creature_manager,
            .level_manager = &level_manager,
            .rendering_manager = &rendering_manager,
            .input_handler = &input_handler,
            .state_manager = &state_manager,
            .menu_manager = &menu_manager,
            .display_manager = &display_manager,
            .game_loop_coordinator = &game_loop_coordinator,
            .data_manager = &data_manager,

            // Specialized systems
            .targeting = &targeting,
            .hunger_system = &hunger_system,
            .buff_system = &buff_system,

            // Game world data
            .stairs = stairs.get(),
            .objects = &objects,
            .inventory_data = &inventory_data,
            .creatures = &creatures,
            .rooms = &rooms,

            // UI
            .menus = &menus,

            // Game state
            .time = &time,
            .run = &run,
            .shouldSave = &shouldSave,
            .isLoadedGame = &isLoadedGame,
            .game_status = &gameStatus,
            .window_state = &windowState
        };
    }

    // Game loop - returns false when game should exit
    bool tick(int& loopNum)
    {
        if (!run) return false;

        windowState = menus.empty() ? WindowState::GAME : WindowState::MENU;
        auto ctx = context();

        switch (windowState)
        {
        case WindowState::MENU:
            menu_manager.handle_menus(menus, ctx);
            break;
        case WindowState::GAME:
            game_loop_coordinator.handle_gameloop(ctx, gui, loopNum);
            break;
        }

        ++loopNum;
        return run;
    }

    // Save on exit if needed
    void shutdown()
    {
        if (shouldSave)
        {
            try
            {
                state_manager.save_game(map, rooms, *player, *stairs, creatures,
                                        inventory_data, gui, hunger_system,
                                        level_manager, time);
            }
            catch (const std::exception& e)
            {
                message_system.log("Error saving: " + std::string(e.what()));
            }
        }
    }

    // Game state
    bool run{ true };
    bool shouldSave{ true };
    bool isLoadedGame{ false };
    int time{ 0 };
    GameStatus gameStatus{ GameStatus::STARTUP };
    WindowState windowState{ WindowState::GAME };

    // Core systems
    RandomDice dice{};
    MessageSystem message_system{};
    RenderingManager rendering_manager{};
    InputHandler input_handler{};
    GameStateManager state_manager{};
    LevelManager level_manager{};
    CreatureManager creature_manager{};
    MenuManager menu_manager{};
    DisplayManager display_manager{};
    GameLoopCoordinator game_loop_coordinator{};
    DataManager data_manager{};
    TargetingSystem targeting{};
    HungerSystem hunger_system{};
    BuffSystem buff_system{};

    // Game world
    Map map{ MAP_HEIGHT, MAP_WIDTH };
    Gui gui{};
    std::unique_ptr<Stairs> stairs{ std::make_unique<Stairs>(Vector2D{0, 0}) };
    std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{0, 0}) };

    std::vector<Vector2D> rooms{};
    std::vector<std::unique_ptr<Creature>> creatures{};
    std::vector<std::unique_ptr<Object>> objects{};
    InventoryData inventory_data{ 1000 };

    // Menu system
    std::deque<std::unique_ptr<BaseMenu>> menus{};
};
