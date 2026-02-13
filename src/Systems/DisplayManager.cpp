// file: Systems/DisplayManager.cpp
#include <algorithm>

#include "DisplayManager.h"
#include "../ActorTypes/Player.h"
#include "../Systems/LevelUpSystem.h"
#include "../UI/LevelUpUI.h"
#include "../UI/CharacterSheetUI.h"

void DisplayManager::display_help() const noexcept
{
    // TODO: stub - help window popup requires curses replacement
    // Previously created a newwin(30, 70, 0, 0) with box, displayed controls text
    // via mvwprintw, waited for getch, then delwin/clear/refresh
}

void DisplayManager::display_levelup(Player& player, int xpLevel, GameContext& ctx) const
{
    // Apply all level up benefits through the new LevelUpSystem
    LevelUpSystem::apply_level_up_benefits(player, xpLevel, &ctx);

    // Display the level up screen using the dedicated UI class
    LevelUpUI::display_level_up_screen(player, xpLevel, ctx);
}

void DisplayManager::display_character_sheet(const Player& player, GameContext& ctx) const noexcept
{
    CharacterSheetUI::display_character_sheet(player, ctx);
}

// end of file: Systems/DisplayManager.cpp
