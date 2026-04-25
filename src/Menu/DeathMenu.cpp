// file: DeathMenu.cpp
#include <cassert>
#include <format>
#include <string>
#include <vector>

#include <raylib.h>

#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Systems/LevelManager.h"
#include "../Systems/MessageSystem.h"
#include "DeathMenu.h"
#include "Menu.h"

static constexpr int MAX_LOG_LINES = 10;
static constexpr int PANEL_W_TILES = 52;
static constexpr int PANEL_H_TILES = 28;

DeathMenu::DeathMenu(GameContext& ctx)
{
    dungeonLevel = ctx.levelManager ? ctx.levelManager->get_dungeon_level() : 0;
    playerLevel = ctx.player ? ctx.player->get_level() : 0;
    playerXp = (ctx.player && ctx.player->destructible) ? ctx.player->destructible->get_xp() : 0;
    killCount = ctx.player ? ctx.player->get_kill_count() : 0;
    playerClass = ctx.player ? ctx.player->get_class_display_name() : "Unknown";
    playerRace = ctx.player ? ctx.player->get_race_display_name() : "Unknown";

    if (ctx.messageSystem)
    {
        size_t count = ctx.messageSystem->get_stored_message_count();
        size_t start = count > MAX_LOG_LINES ? count - MAX_LOG_LINES : 0;
        for (size_t i = start; i < count; ++i)
        {
            const auto& parts = ctx.messageSystem->get_attack_message_at(i);
            std::string line{};
            for (const auto& part : parts)
            {
                line += part.logMessageText;
            }
            if (!line.empty())
            {
                recentMessages.push_back(std::move(line));
            }
        }
    }
}

void DeathMenu::menu(GameContext& ctx)
{
    menu_new(PANEL_W_TILES, PANEL_H_TILES, 0, 0, ctx);
    menu_key_listen();
    menu_clear();
    render(ctx);
    menu_refresh();
    handle_input(ctx);
}

void DeathMenu::render(GameContext& ctx) const
{
    assert(ctx.renderer && "DeathMenu::render called without a renderer");

    Renderer& r = *ctx.renderer;
    const int ts = r.get_tile_size();
    const int cols = r.get_viewport_cols();
    const int rows = r.get_viewport_rows();

    const int startX = ((cols - PANEL_W_TILES) / 2) * ts;
    const int startY = ((rows - PANEL_H_TILES) / 2) * ts;

    r.draw_frame(Vector2D{ startX, startY }, PANEL_W_TILES, PANEL_H_TILES, *ctx.tileConfig);

    const int textX = startX + ts;
    int textY = startY + ts;
    const int lineH = ts;

    // Title
    std::string_view title = "* YOU HAVE DIED *";
    int titleW = r.measure_text(title);
    int titleX = startX + (PANEL_W_TILES * ts - titleW) / 2;
    r.draw_text_color(Vector2D{ titleX, textY }, title, RED);
    textY += lineH * 2;

    // Stats
    r.draw_text_color(
        Vector2D{ textX, textY },
        std::format("{} {} - Level {}", playerRace, playerClass, playerLevel),
        WHITE);
    textY += lineH;

    r.draw_text_color(
        Vector2D{ textX, textY },
        std::format("Dungeon Level : {}", dungeonLevel),
        YELLOW);
    textY += lineH;

    r.draw_text_color(
        Vector2D{ textX, textY },
        std::format("Monsters Slain: {}", killCount),
        YELLOW);
    textY += lineH;

    r.draw_text_color(
        Vector2D{ textX, textY },
        std::format("Experience    : {}", playerXp),
        YELLOW);
    textY += lineH * 2;

    // Log header
    r.draw_text_color(Vector2D{ textX, textY }, "-- Last Messages --", GRAY);
    textY += lineH;

    for (const auto& line : recentMessages)
    {
        r.draw_text_color(Vector2D{ textX, textY }, line, LIGHTGRAY);
        textY += lineH;
    }

    // Prompt
    std::string_view prompt = "[ ENTER ] Return to main menu";
    int promptW = r.measure_text(prompt);
    int promptX = startX + (PANEL_W_TILES * ts - promptW) / 2;
    int promptY = startY + (PANEL_H_TILES - 2) * ts;
    r.draw_text_color(Vector2D{ promptX, promptY }, prompt, Color{ 0, 255, 255, 255 });
}

void DeathMenu::handle_input(GameContext& ctx)
{
    if (keyPress == 10) // ENTER
    {
        run = false;
        ctx.menus->clear();
        ctx.menus->push_back(make_main_menu(true, ctx));
        keyPress = 0;
    }
}
