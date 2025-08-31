// file: Systems/MenuManager.cpp
#include "MenuManager.h"
#include "../Menu/BaseMenu.h"
#include "../Game.h"

void MenuManager::handle_menus(std::deque<std::unique_ptr<BaseMenu>>& menus)
{
    if (!menus.empty())
    {
        bool menuWasPopped = false;
        
        menus.back()->menu();
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
            restore_game_display();
        }

        shouldTakeInput = false;
    }
}

bool MenuManager::has_active_menus(const std::deque<std::unique_ptr<BaseMenu>>& menus) const noexcept
{
    return !menus.empty();
}

void MenuManager::restore_game_display()
{
    game.restore_game_display();
}

// end of file: Systems/MenuManager.cpp
