#pragma once

#include <vector>
#include <memory>
#include <deque>

// Forward declarations
class Game;
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
class Stairs;
class Object;
class BaseMenu;
struct Vector2D;

// Game status enumeration - moved here to avoid circular dependency
enum class GameStatus
{
    STARTUP,
    IDLE,
    NEW_TURN,
    VICTORY,
    DEFEAT
};

enum class WindowState { MENU, GAME };

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
    RandomDice* dice_roller{ nullptr };  // Alias for dice (backward compatibility)

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
    TargetingSystem* targeting_system{ nullptr };  // Alias for targeting (backward compatibility)
    HungerSystem* hunger_system{ nullptr };

    // Game world data
    Stairs* stairs{ nullptr };
    std::vector<std::unique_ptr<Object>>* objects{ nullptr };
    class InventoryData* inventory_data{ nullptr };
    std::vector<std::unique_ptr<class Creature>>* creatures{ nullptr };
    std::vector<Vector2D>* rooms{ nullptr };

    // UI Collections
    std::deque<std::unique_ptr<BaseMenu>>* menus{ nullptr };

    // Game state (pointers to allow mutation)
    int* time{ nullptr };
    bool* run{ nullptr };
    bool* shouldSave{ nullptr };
    bool* isLoadedGame{ nullptr };  // Track new game vs loaded game
    GameStatus* game_status{ nullptr };
    WindowState* window_state{ nullptr };
};
