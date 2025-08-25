// LevelUpUI.cpp - Handles level up screen display

#include "LevelUpUI.h"
#include "../ActorTypes/Player.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include "../Systems/LevelUpSystem.h"
#include "../dnd_tables/CalculatedTHAC0s.h"
#include <format>

void LevelUpUI::display_level_up_screen(Player& player, int newLevel)
{
    // Clear screen before showing level up window
    clear();
    refresh();

    // Create window for level up display
    WINDOW* statsWindow = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_Y, WINDOW_X);
    box(statsWindow, 0, 0);

    // Display all sections
    display_title(statsWindow, player, newLevel);
    display_basic_info(statsWindow, player, newLevel);
    display_current_stats(statsWindow, player);
    
    int currentLine = 12; // Start benefits section here
    display_level_benefits(statsWindow, player, newLevel);
    display_class_benefits(statsWindow, player, newLevel, currentLine);
    display_next_level_info(statsWindow, player);
    display_continue_prompt(statsWindow);

    // Refresh and wait for input
    wrefresh(statsWindow);
    wait_for_spacebar();

    // Clean up
    delwin(statsWindow);
    cleanup_and_restore();
}

void LevelUpUI::display_title(WINDOW* window, const Player& player, int level)
{
    wattron(window, A_BOLD);
    mvwprintw(window, 1, 20, "LEVEL UP: %s", player.playerClass.c_str());
    wattroff(window, A_BOLD);
}

void LevelUpUI::display_basic_info(WINDOW* window, const Player& player, int level)
{
    mvwprintw(window, 3, 2, "Name: %s", player.actorData.name.c_str());
    mvwprintw(window, 3, 30, "Race: %s", player.playerRace.c_str());
    mvwprintw(window, 4, 2, "Level: %d", level);
    mvwprintw(window, 4, 30, "Experience: %d", player.destructible->xp);
}

void LevelUpUI::display_current_stats(WINDOW* window, const Player& player)
{
    mvwprintw(window, 6, 2, "CURRENT STATS:");
    mvwprintw(window, 7, 4, "Hit Points: %d/%d", player.destructible->hp, player.destructible->hpMax);
    mvwprintw(window, 8, 4, "THAC0: %d", player.destructible->thaco);
    mvwprintw(window, 9, 4, "Armor Class: %d", player.destructible->armorClass);
    mvwprintw(window, 10, 4, "Damage Reduction: %d", player.destructible->dr);
}

void LevelUpUI::display_level_benefits(WINDOW* window, const Player& player, int level)
{
    mvwprintw(window, 12, 2, "AD&D 2e LEVEL %d BENEFITS:", level);

    // Show HP gain this level
    wattron(window, COLOR_PAIR(GREEN_BLACK_PAIR));
    mvwprintw(window, 13, 4, "+ Hit Points gained this level");
    wattroff(window, COLOR_PAIR(GREEN_BLACK_PAIR));
    
    // Show THAC0 improvement if any
    if (has_thac0_improvement(player, level))
    {
        wattron(window, COLOR_PAIR(GREEN_BLACK_PAIR));
        mvwprintw(window, 14, 4, "+ THAC0 improved to %d", player.destructible->thaco);
        wattroff(window, COLOR_PAIR(GREEN_BLACK_PAIR));
    }
    
    // Show ability score improvement at levels 4, 8, 12, 16, 20
    if (level % 4 == 0)
    {
        wattron(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
        mvwprintw(window, 15, 4, "+ Ability Score Improvement!");
        wattroff(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
    }
}

void LevelUpUI::display_class_benefits(WINDOW* window, const Player& player, int level, int& currentLine)
{
    int benefitLine = 16;
    
    switch (player.playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        mvwprintw(window, benefitLine++, 4, "Class: Fighter (d10 hit dice)");
        if (level == 7)
        {
            wattron(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
            mvwprintw(window, benefitLine++, 4, "+ SPECIAL: Extra Attack (3/2 per round)!");
            wattroff(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
        }
        else if (level == 13)
        {
            wattron(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
            mvwprintw(window, benefitLine++, 4, "+ SPECIAL: Extra Attack (2 per round)!");
            wattroff(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
        }
        break;

    case Player::PlayerClassState::ROGUE:
        mvwprintw(window, benefitLine++, 4, "Class: Rogue (d6 hit dice)");
        if (LevelUpSystem::calculate_backstab_multiplier(level) > LevelUpSystem::calculate_backstab_multiplier(level - 1))
        {
            wattron(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
            mvwprintw(
                window,
                benefitLine++,
                4,
                "+ SPECIAL: Backstab multiplier x%d!", 
                LevelUpSystem::calculate_backstab_multiplier(level)
            );
            wattroff(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
        }
        break;

    case Player::PlayerClassState::CLERIC:
        mvwprintw(window, benefitLine++, 4, "Class: Cleric (d8 hit dice)");
        if (level == 3 || level == 5 || level == 7 || level == 9)
        {
            wattron(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
            mvwprintw(window, benefitLine++, 4, "+ SPECIAL: Turn Undead improved!");
            wattroff(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
        }
        break;

    case Player::PlayerClassState::WIZARD:
        mvwprintw(window, benefitLine++, 4, "Class: Wizard (d4 hit dice)");
        if ((level % 2 == 1) && level > 1)
        {
            int spellLevel = (level + 1) / 2;
            if (spellLevel <= 9)
            {
                wattron(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
                mvwprintw(window, benefitLine++, 4, "+ SPECIAL: Level %d spells available!", spellLevel);
                wattroff(window, COLOR_PAIR(YELLOW_BLACK_PAIR));
            }
        }
        break;

    default:
        mvwprintw(window, benefitLine++, 4, "Class: Unknown");
        break;
    }
    
    currentLine = benefitLine;
}

void LevelUpUI::display_next_level_info(WINDOW* window, const Player& player)
{
    // Show next level requirements
    int nextLevelXP = player.ai->get_next_level_xp(const_cast<Player&>(player));
    mvwprintw(window, 17, 2, "XP for next level: %d", nextLevelXP);
}

void LevelUpUI::display_continue_prompt(WINDOW* window)
{
    // Prompt specifically for space bar
    mvwprintw(window, 19, 15, "Press SPACE BAR to continue...");
}

bool LevelUpUI::has_thac0_improvement(const Player& player, int level)
{
    int expectedTHAC0 = get_expected_thac0(player, level);
    return player.destructible->thaco <= expectedTHAC0;
}

int LevelUpUI::get_expected_thac0(const Player& player, int level)
{
    CalculatedTHAC0s thac0Tables;
    
    switch (player.playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        return thac0Tables.getFighter(level);
    case Player::PlayerClassState::ROGUE:
        return thac0Tables.getRogue(level);
    case Player::PlayerClassState::CLERIC:
        return thac0Tables.getCleric(level);
    case Player::PlayerClassState::WIZARD:
        return thac0Tables.getWizard(level);
    default:
        return 20;
    }
}

void LevelUpUI::wait_for_spacebar()
{
    int ch;
    do {
        ch = getch();
    } while (ch != ' '); // Only accept space bar
}

void LevelUpUI::cleanup_and_restore()
{
    // Clear screen and restore game display properly
    clear();
    game.render();
    game.gui->gui_render();
    refresh();
}
