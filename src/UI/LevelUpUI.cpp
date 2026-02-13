// LevelUpUI.cpp - Handles level up screen display
#include <format>

#include "LevelUpUI.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Systems/LevelUpSystem.h"
#include "../dnd_tables/CalculatedTHAC0s.h"
#include "../Core/GameContext.h"
#include "../Systems/RenderingManager.h"
#include "../Gui/Gui.h"

void LevelUpUI::display_level_up_screen(Player& player, int newLevel, GameContext& ctx)
{
    // TODO: render level up window via new renderer
    void* statsWindow = nullptr;

    // Display all sections (currently no-ops pending renderer)
    display_title(statsWindow, player, newLevel);
    display_basic_info(statsWindow, player, newLevel);
    display_current_stats(statsWindow, player);

    int currentLine = 12; // Start benefits section here
    display_level_benefits(statsWindow, player, newLevel);
    display_class_benefits(statsWindow, player, newLevel, currentLine);
    display_next_level_info(statsWindow, player, ctx);
    display_continue_prompt(statsWindow);

    // Wait for input then clean up
    wait_for_spacebar();
    cleanup_and_restore(ctx);
}

void LevelUpUI::display_title(void* window, const Player& player, int level)
{
    // TODO: render title via new renderer
    (void)window;
    (void)player;
    (void)level;
}

void LevelUpUI::display_basic_info(void* window, const Player& player, int level)
{
    // TODO: render basic info via new renderer
    (void)window;
    (void)player;
    (void)level;
}

void LevelUpUI::display_current_stats(void* window, const Player& player)
{
    // TODO: render current stats via new renderer
    (void)window;
    (void)player;
}

void LevelUpUI::display_level_benefits(void* window, const Player& player, int level)
{
    // TODO: render level benefits via new renderer
    (void)window;
    (void)player;
    (void)level;
}

void LevelUpUI::display_class_benefits(void* window, const Player& player, int level, int& currentLine)
{
    int benefitLine = 16;

    switch (player.playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        // Fighter: d10 hit dice, extra attacks at 7 and 13
        if (level == 7)
        {
            // SPECIAL: Extra Attack (3/2 per round)
        }
        else if (level == 13)
        {
            // SPECIAL: Extra Attack (2 per round)
        }
        benefitLine++;
        break;

    case Player::PlayerClassState::ROGUE:
        // Rogue: d6 hit dice, backstab multiplier increases
        if (LevelUpSystem::calculate_backstab_multiplier(level) > LevelUpSystem::calculate_backstab_multiplier(level - 1))
        {
            // SPECIAL: Backstab multiplier increased
            benefitLine++;
        }
        benefitLine++;
        break;

    case Player::PlayerClassState::CLERIC:
        // Cleric: d8 hit dice, Turn Undead improvements
        if (level == 3 || level == 5 || level == 7 || level == 9)
        {
            // SPECIAL: Turn Undead improved
            benefitLine++;
        }
        benefitLine++;
        break;

    case Player::PlayerClassState::WIZARD:
        // Wizard: d4 hit dice, new spell levels at odd levels
        if ((level % 2 == 1) && level > 1)
        {
            int spellLevel = (level + 1) / 2;
            if (spellLevel <= 9)
            {
                // SPECIAL: New spell level available
                benefitLine++;
            }
        }
        benefitLine++;
        break;

    default:
        benefitLine++;
        break;
    }

    currentLine = benefitLine;

    // TODO: render class benefits via new renderer
    (void)window;
}

void LevelUpUI::display_next_level_info(void* window, const Player& player, GameContext& ctx)
{
    // Show next level requirements
    int nextLevelXP = player.ai->get_next_level_xp(ctx, const_cast<Player&>(player));

    // TODO: render next level info via new renderer
    (void)window;
    (void)nextLevelXP;
}

void LevelUpUI::display_continue_prompt(void* window)
{
    // TODO: render continue prompt via new renderer
    (void)window;
}

bool LevelUpUI::has_thac0_improvement(const Player& player, int level)
{
    int expectedTHAC0 = get_expected_thac0(player, level);
    return player.destructible->get_thaco() <= expectedTHAC0;
}

int LevelUpUI::get_expected_thac0(const Player& player, int level)
{
    CalculatedTHAC0s thac0Tables;

    switch (player.playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        return thac0Tables.get_fighter(level);
    case Player::PlayerClassState::ROGUE:
        return thac0Tables.get_rogue(level);
    case Player::PlayerClassState::CLERIC:
        return thac0Tables.get_cleric(level);
    case Player::PlayerClassState::WIZARD:
        return thac0Tables.get_wizard(level);
    default:
        return 20;
    }
}

void LevelUpUI::wait_for_spacebar()
{
    // TODO: wait for spacebar via new input system
}

void LevelUpUI::cleanup_and_restore(GameContext& ctx)
{
    // Restore game display
    ctx.rendering_manager->render(ctx);
    ctx.gui->gui_render(ctx);
}
