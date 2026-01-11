// file: Systems/MenuManager.cpp
#include "MenuManager.h"
#include "../Menu/BaseMenu.h"
#include "../Core/GameContext.h"
#include "../Game.h"

void MenuManager::handle_menus(std::deque<std::unique_ptr<BaseMenu>>& menus, GameContext& ctx)
{
    if (!menus.empty())
    {
        bool menuWasPopped = false;
        
        menus.back()->menu(ctx);
        // if back is pressed, pop the menu
        if (menus.back()->back)
        {
            menus.pop_back();
            menuWasPopped = true;
            if (!menus.empty())
            {
                menus.back()->menu_set_run_true();
            }
        }

        if (!menus.empty() && !menus.back()->run)
        {
            menus.pop_back();
            menuWasPopped = true;
        }

        // If we just closed a menu and returned to game, restore display
        if (menuWasPopped && menus.empty() && gameWasInit)
        {
            restore_game_display(ctx);
        }

        shouldTakeInput = false;
    }
}

bool MenuManager::has_active_menus(const std::deque<std::unique_ptr<BaseMenu>>& menus) const noexcept
{
    return !menus.empty();
}

void MenuManager::restore_game_display(GameContext& ctx)
{
    ctx.game->restore_game_display();
}

// end of file: Systems/MenuManager.cpp
