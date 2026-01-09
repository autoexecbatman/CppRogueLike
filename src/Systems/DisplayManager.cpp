// file: Systems/DisplayManager.cpp
#include "DisplayManager.h"
#include "../ActorTypes/Player.h"
#include "../Systems/LevelUpSystem.h"
#include "../UI/LevelUpUI.h"
#include "../UI/CharacterSheetUI.h"
#include <curses.h>

void DisplayManager::display_help() const noexcept
{
    WINDOW* help_window = newwin(
        30, // height
        70, // width
        0,  // y
        0   // x
    );
    box(help_window, 0, 0);
    refresh();
    bool run{ true };
    while (run == true)
    {
        mvwprintw(help_window, 1, 1, "=== CONTROLS ===");
        mvwprintw(help_window, 3, 1, "Movement: Arrow keys, WASD (W/A/S/D)");
        mvwprintw(help_window, 4, 1, "Diagonal: (Q/E/Z/C)");
        mvwprintw(help_window, 5, 1, "Wait: 'h'");
        mvwprintw(help_window, 6, 1, "Pick up item: 'p'");
        mvwprintw(help_window, 7, 1, "Drop item: 'l'");
        mvwprintw(help_window, 8, 1, "Inventory: 'i'");
        mvwprintw(help_window, 9, 1, "Character sheet: '@'");
        mvwprintw(help_window, 10, 1, "Ranged attack or look around: 't' (requires ranged weapon to shoot)");
        mvwprintw(help_window, 12, 1, "Open door: 'o'");
        mvwprintw(help_window, 13, 1, "Close door: 'k'");
        mvwprintw(help_window, 14, 1, "Rest: 'r' (recovers health but costs food)");
        mvwprintw(help_window, 15, 1, "Descend stairs: '>'");
        mvwprintw(help_window, 16, 1, "Help: '?'");
        mvwprintw(help_window, 17, 1, "Menu: 'Esc'");
        mvwprintw(help_window, 18, 1, "Quit: '~'");
        mvwprintw(help_window, 19, 1, "=== RESTING ===");
        mvwprintw(help_window, 20, 1, "- Resting recovers 20 percent of your maximum health");
        mvwprintw(help_window, 21, 1, "- You cannot rest when enemies are nearby (within 5 tiles)");
        mvwprintw(help_window, 22, 1, "- Resting increases hunger, so make sure to have food");
        mvwprintw(help_window, 23, 1, "- You cannot rest if you're starving");
        mvwprintw(help_window, 28, 1, "Press any key to close this window");
        wrefresh(help_window);
        const int key = getch();
        // If any key was pressed then exit the loop
        if (key != ERR) {
            run = false;
        }
    }
    delwin(help_window);
    clear();
    refresh();
}

void DisplayManager::display_levelup(Player& player, int xpLevel, GameContext& ctx) const
{
    // Apply all level up benefits through the new LevelUpSystem
    LevelUpSystem::apply_level_up_benefits(player, xpLevel, &ctx);

    // Display the level up screen using the dedicated UI class
    LevelUpUI::display_level_up_screen(player, xpLevel);
}

void DisplayManager::display_character_sheet(const Player& player) const noexcept
{
    CharacterSheetUI::display_character_sheet(player);
}

// end of file: Systems/DisplayManager.cpp
