// LevelUpUI.cpp - Handles level up screen display
#include <format>

#include "LevelUpUI.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Systems/LevelUpSystem.h"
#include "../dnd_tables/CalculatedTHAC0s.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/InputSystem.h"

void LevelUpUI::display_level_up_screen(Player& player, int newLevel, GameContext& ctx)
{
    bool run = true;
    while (run && !WindowShouldClose())
    {
        ctx.renderer->begin_frame();

        int row = 0;
        draw_title(player, newLevel, ctx, row);
        draw_current_stats(player, ctx, row);
        draw_level_benefits(player, newLevel, ctx, row);
        draw_next_level_info(player, ctx, row);

        int ts = ctx.renderer->get_tile_size();
        ctx.renderer->draw_text(ts, row * ts, "Press [SPACE] to continue", CYAN_BLACK_PAIR);

        ctx.renderer->end_frame();

        ctx.input_system->poll();
        GameKey key = ctx.input_system->get_key();
        if (key == GameKey::SPACE || key == GameKey::ESCAPE || key == GameKey::ENTER)
        {
            run = false;
        }
    }
}

void LevelUpUI::draw_title(const Player& player, int level, GameContext& ctx, int& row)
{
    int ts = ctx.renderer->get_tile_size();

    ctx.renderer->draw_text(ts, row * ts, "*** LEVEL UP! ***", YELLOW_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        ts, row * ts,
        std::format("{} has attained level {}!", player.get_name(), level),
        WHITE_BLACK_PAIR
    );
    row++;

    ctx.renderer->draw_text(
        ts, row * ts,
        std::format("Class: {}", player.playerClass),
        WHITE_BLACK_PAIR
    );
    row += 2;
}

void LevelUpUI::draw_current_stats(const Player& player, GameContext& ctx, int& row)
{
    int ts = ctx.renderer->get_tile_size();

    ctx.renderer->draw_text(ts, row * ts, "--- CURRENT STATS ---", YELLOW_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        ts, row * ts,
        std::format("HP: {} / {}   THAC0: {}   AC: {}",
            player.destructible->get_hp(),
            player.destructible->get_max_hp(),
            player.destructible->get_thaco(),
            player.destructible->get_armor_class()),
        GREEN_BLACK_PAIR
    );
    row += 2;
}

void LevelUpUI::draw_level_benefits(const Player& player, int level, GameContext& ctx, int& row)
{
    int ts = ctx.renderer->get_tile_size();

    ctx.renderer->draw_text(ts, row * ts, "--- LEVEL BENEFITS ---", YELLOW_BLACK_PAIR);
    row++;

    if (has_thac0_improvement(player, level))
    {
        ctx.renderer->draw_text(
            ts, row * ts,
            std::format("THAC0 improved to {}", player.destructible->get_thaco()),
            GREEN_BLACK_PAIR
        );
        row++;
    }

    switch (player.playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
    {
        ctx.renderer->draw_text(ts, row * ts, "Hit Dice: d10", WHITE_BLACK_PAIR);
        row++;
        if (level == 7)
        {
            ctx.renderer->draw_text(
                ts, row * ts,
                "Special: Extra Attack (3 attacks per 2 rounds)",
                YELLOW_BLACK_PAIR
            );
            row++;
        }
        else if (level == 13)
        {
            ctx.renderer->draw_text(
                ts, row * ts,
                "Special: Extra Attack (2 attacks per round)",
                YELLOW_BLACK_PAIR
            );
            row++;
        }
        break;
    }
    case Player::PlayerClassState::ROGUE:
    {
        ctx.renderer->draw_text(ts, row * ts, "Hit Dice: d6", WHITE_BLACK_PAIR);
        row++;
        int newMult = LevelUpSystem::calculate_backstab_multiplier(level);
        int oldMult = LevelUpSystem::calculate_backstab_multiplier(level - 1);
        if (newMult > oldMult)
        {
            ctx.renderer->draw_text(
                ts, row * ts,
                std::format("Special: Backstab multiplier increased to x{}", newMult),
                YELLOW_BLACK_PAIR
            );
            row++;
        }
        break;
    }
    case Player::PlayerClassState::CLERIC:
    {
        ctx.renderer->draw_text(ts, row * ts, "Hit Dice: d8", WHITE_BLACK_PAIR);
        row++;
        if (level == 3 || level == 5 || level == 7 || level == 9)
        {
            ctx.renderer->draw_text(
                ts, row * ts,
                "Special: Turn Undead ability improved",
                YELLOW_BLACK_PAIR
            );
            row++;
        }
        break;
    }
    case Player::PlayerClassState::WIZARD:
    {
        ctx.renderer->draw_text(ts, row * ts, "Hit Dice: d4", WHITE_BLACK_PAIR);
        row++;
        if ((level % 2 == 1) && level > 1)
        {
            int spellLevel = (level + 1) / 2;
            if (spellLevel <= 9)
            {
                ctx.renderer->draw_text(
                    ts, row * ts,
                    std::format("Special: Level {} spells now available", spellLevel),
                    YELLOW_BLACK_PAIR
                );
                row++;
            }
        }
        break;
    }
    default:
        break;
    }

    row++;
}

void LevelUpUI::draw_next_level_info(const Player& player, GameContext& ctx, int& row)
{
    int ts = ctx.renderer->get_tile_size();

    int nextLevelXP = player.ai->get_next_level_xp(ctx, const_cast<Player&>(player));

    ctx.renderer->draw_text(ts, row * ts, "--- NEXT LEVEL ---", YELLOW_BLACK_PAIR);
    row++;

    ctx.renderer->draw_text(
        ts, row * ts,
        std::format("XP needed for next level: {}", nextLevelXP),
        WHITE_BLACK_PAIR
    );
    row += 2;
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
