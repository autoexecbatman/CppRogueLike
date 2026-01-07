// file: Systems/MenuManager.h
#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#pragma once

#include <deque>
#include <memory>

class BaseMenu;
struct GameContext;

class MenuManager
{
public:
    MenuManager() = default;
    ~MenuManager() = default;

    // Core menu management
    void handle_menus(std::deque<std::unique_ptr<BaseMenu>>& menus, GameContext& ctx);
    
    // Menu state queries
    bool has_active_menus(const std::deque<std::unique_ptr<BaseMenu>>& menus) const noexcept;
    
    // Game state accessors
    bool is_game_initialized() const noexcept { return gameWasInit; }
    void set_game_initialized(bool initialized) noexcept { gameWasInit = initialized; }
    
    bool should_take_input() const noexcept { return shouldTakeInput; }
    void set_should_take_input(bool takeInput) noexcept { shouldTakeInput = takeInput; }

private:
    bool gameWasInit{ false };
    bool shouldTakeInput{ true };

    void restore_game_display(GameContext& ctx);
};

#endif // MENU_MANAGER_H
// end of file: Systems/MenuManager.h
