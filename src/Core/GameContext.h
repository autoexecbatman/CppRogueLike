#pragma once

// Forward declarations
class Map;
class Gui;
class MessageSystem;
class RandomDice;
class CreatureManager;
class LevelManager;
class RenderingManager;
class InputHandler;
class GameStateManager;
class MenuManager;
class DisplayManager;
class GameLoopCoordinator;
class DataManager;
class TargetingSystem;
class HungerSystem;
class Player;

/**
 * GameContext - Dependency injection container
 *
 * Replaces global `extern Game game;` with explicit dependency passing.
 * Phase 1: Expand structure with all Game systems
 * Phase 2: Add get_context() to Game class
 * Phase 3: Replace game.X with ctx->X incrementally (1,196 references)
 */
struct GameContext {
    // Core game world
    Map* map{ nullptr };
    Gui* gui{ nullptr };
    Player* player{ nullptr };

    // Core systems
    MessageSystem* message_system{ nullptr };
    RandomDice* dice{ nullptr };

    // Managers
    CreatureManager* creature_manager{ nullptr };
    LevelManager* level_manager{ nullptr };
    RenderingManager* rendering_manager{ nullptr };
    InputHandler* input_handler{ nullptr };
    GameStateManager* state_manager{ nullptr };
    MenuManager* menu_manager{ nullptr };
    DisplayManager* display_manager{ nullptr };
    GameLoopCoordinator* game_loop_coordinator{ nullptr };
    DataManager* data_manager{ nullptr };

    // Specialized systems
    TargetingSystem* targeting{ nullptr };
    HungerSystem* hunger_system{ nullptr };

    // Game state (pointer to allow mutation)
    int* time{ nullptr };
    bool* run{ nullptr };
};
