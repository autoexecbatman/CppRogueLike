// file: Game.h
#ifndef GAME_H
#define GAME_H

#pragma once

// Standard library
#include <iostream>
#include <memory>
#include <span>
#include <deque>
#include <vector>

// Core game components
#include "Map/Map.h"
#include "ActorTypes/Player.h"
#include "Actor/InventoryData.h"
#include "Gui/Gui.h"
#include "Menu/BaseMenu.h"
#include "Random/RandomDice.h"
#include "Core/GameContext.h"

// Systems - All game logic now delegated to these systems
#include "Systems/TargetingSystem.h"
#include "Systems/HungerSystem.h"
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

// Data structures
#include "Attributes/StrengthAttributes.h"
#include "Attributes/DexterityAttributes.h"
#include "Attributes/ConstitutionAttributes.h"
#include "Attributes/CharismaAttributes.h"
#include "Attributes/IntelligenceAttributes.h"
#include "Attributes/WisdomAttributes.h"
#include "Items/Weapons.h"
#include "Actor/InventoryData.h"

// Forward declarations
class Creature;
class Object;
class Web;
class Stairs;

class Game
{
public:
    // Core game state
    bool run{ true };
    bool shouldSave{ true };
    int time{ 0 };

    enum class GameStatus
    {
        STARTUP, IDLE, NEW_TURN, VICTORY, DEFEAT
    }
    gameStatus{ GameStatus::STARTUP };

    enum class WindowState
    {
        MENU, GAME
    }
    windowState{ WindowState::GAME };

    // Core systems - All game logic delegated to specialized systems
    RandomDice d{};
    TargetingSystem targeting{};
    HungerSystem hunger_system{};
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

    // Game world data
    Map map{ Map{MAP_HEIGHT, MAP_WIDTH} };
    Gui gui{};
    std::unique_ptr<Stairs> stairs{ std::make_unique<Stairs>(Vector2D{0, 0}) };
    std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{0, 0}) };

    std::vector<Vector2D> rooms;
    std::vector<std::unique_ptr<Creature>> creatures;
    std::vector<std::unique_ptr<Object>> objects;
    InventoryData inventory_data{1000}; // Floor items container

    // Menu system
    std::deque<std::unique_ptr<BaseMenu>> menus;
    std::deque<std::unique_ptr<BaseMenu>> deadMenus;

    // Core game methods - minimal interface, everything delegated to systems
    void init();
    void update();

    // Dependency injection - Phase 2: Provide context for refactored code
    GameContext get_context() noexcept;

    // System delegations - clean interface
    void render() const { rendering_manager.render_world(map, *stairs, objects, inventory_data, creatures, *player); }
    void handle_menus() { auto ctx = get_context(); menu_manager.handle_menus(menus, ctx); }
    void handle_gameloop(Gui& gui, int loopNum) { auto ctx = get_context(); game_loop_coordinator.handle_gameloop(ctx, gui, loopNum); }
    void handle_ranged_attack() { auto ctx = get_context(); targeting.handle_ranged_attack(ctx); }
    void display_help() noexcept { display_manager.display_help(); }
    void display_levelup(int level) { display_manager.display_levelup(*player, level); }
    void display_character_sheet() const noexcept { display_manager.display_character_sheet(*player); }
    bool pick_tile(Vector2D* position, int maxRange) { auto ctx = get_context(); return targeting.pick_tile(ctx, position, maxRange); }

    // Creature management
    void update_creatures(std::span<std::unique_ptr<Creature>> creatures) { creature_manager.update_creatures(creatures); }
    void cleanup_dead_creatures() { creature_manager.cleanup_dead_creatures(creatures); }
    void spawn_creatures() { creature_manager.spawn_creatures(creatures, rooms, map, d, time); }
    template<typename T>
    void send_to_back(T& actor) { creature_manager.send_to_back(creatures, actor); }
    Creature* get_closest_monster(Vector2D fromPosition, double inRange) const noexcept { return creature_manager.get_closest_monster(creatures, fromPosition, inRange); }
    Creature* get_actor(Vector2D pos) const noexcept { return creature_manager.get_actor_at_position(creatures, pos); }

    // Game state management
    void load_all();
    void save_all();
    void next_level() { level_manager.advance_to_next_level(map, *player, message_system); gameStatus = GameStatus::STARTUP; }

    // Emscripten compatibility
    void safe_screen_clear() { rendering_manager.safe_screen_clear(); }
    void force_screen_refresh() { rendering_manager.force_screen_refresh(); }
    void restore_game_display() { render(); gui.gui_render(); rendering_manager.force_screen_refresh(); }

    // Debug utilities
    void wizard_eye() noexcept;
    void add_debug_weapons_at_player_feet();
    void err(std::string_view e) noexcept { if (message_system.is_debug_mode()) { clear(); mvprintw(MAP_HEIGHT / 2, MAP_WIDTH / 2, e.data()); refresh(); getch(); } }

    // Message system delegation
    void message(int color, std::string_view text, bool isComplete = false) { message_system.message(color, text, isComplete); }
    void append_message_part(int color, std::string_view text) { message_system.append_message_part(color, text); }
    void finalize_message() { message_system.finalize_message(); }
    void transfer_messages_to_gui() { message_system.transfer_messages_to_gui(gui); }
    void log(std::string_view message) const { message_system.log(message); }
    void display_debug_messages() noexcept { message_system.display_debug_messages(); }
    void enable_debug_mode() noexcept { message_system.enable_debug_mode(); }
    void disable_debug_mode() noexcept { message_system.disable_debug_mode(); }

private:
    bool computeFov{ false };
};

// Global game instance
extern Game game;

// Utility template
template<typename T>
void print_container(std::span<std::unique_ptr<T>> container)
{
    int i = 0;
    for (const auto& item : container)
    {
        std::cout << i << ". " << item->name << " ";
        i++;
    }
    std::cout << '\n';
}

#endif // GAME_H
// end of file: Game.h
