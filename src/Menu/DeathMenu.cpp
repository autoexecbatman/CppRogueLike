// file: DeathMenu.cpp
#include <algorithm>
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
#include "../ActorTypes/Player.h"

static constexpr int MAX_LOG_LINES = 8;
// The panel is sized to the viewport rather than to a fixed tile count. At 52 by
// 28 tiles it was 3328 by 1792 pixels on a 1280 by 896 canvas, so the centring
// arithmetic produced a negative origin and every line but the two centred ones
// was drawn off the left edge of the screen.
static constexpr int PANEL_MARGIN_TILES = 2;
static constexpr int PANEL_MAX_W_TILES = 16;
static constexpr int PANEL_MAX_H_TILES = 11;
// Text rows are 32 pixels, not a whole 64-pixel tile: the font is 16, so a tile
// per line spent three quarters of the panel on nothing and needed a panel twice
// the height of the screen to hold a dozen lines.
static constexpr int TEXT_ROW_PITCH = 32;

// How many tiles wide and tall the panel gets on this viewport.
static int panel_width_tiles(int viewportCols)
{
	return std::min(PANEL_MAX_W_TILES, viewportCols - PANEL_MARGIN_TILES * 2);
}

static int panel_height_tiles(int viewportRows)
{
	return std::min(PANEL_MAX_H_TILES, viewportRows - PANEL_MARGIN_TILES * 2);
}

DeathMenu::DeathMenu(GameContext& ctx)
{
    dungeonLevel = ctx.levelManager ? ctx.levelManager->get_dungeon_level() : 0;
    playerLevel = ctx.player() ? ctx.player()->get_level() : 0;
    playerXp = ctx.player() ? ctx.player()->get_xp() : 0;
    killCount = ctx.player() ? ctx.player_concrete().get_kill_count() : 0;
    playerClass = ctx.player() ? ctx.player_concrete().get_class_display_name() : "Unknown";
    playerRace = ctx.player() ? ctx.player_concrete().get_race_display_name() : "Unknown";

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
    assert(ctx.renderer && "DeathMenu::menu called without a renderer");
    menu_new(
        panel_width_tiles(ctx.renderer->get_viewport_cols()),
        panel_height_tiles(ctx.renderer->get_viewport_rows()),
        0,
        0,
        ctx);
    menu_key_listen();
    menu_clear();
    render(ctx);
    menu_refresh();
    handle_input(ctx);
}

void DeathMenu::render(GameContext& ctx) const
{
    assert(ctx.renderer && "DeathMenu::render called without a renderer");

    Renderer& renderer = *ctx.renderer;
    const int tileSize = renderer.get_tile_size();
    const int panelWidthTiles = panel_width_tiles(renderer.get_viewport_cols());
    const int panelHeightTiles = panel_height_tiles(renderer.get_viewport_rows());
    const int panelWidth = panelWidthTiles * tileSize;
    const int panelHeight = panelHeightTiles * tileSize;

    const int startX = (renderer.get_viewport_cols() - panelWidthTiles) / 2 * tileSize;
    const int startY = (renderer.get_viewport_rows() - panelHeightTiles) / 2 * tileSize;

    renderer.draw_frame(Vector2D{ startX, startY }, panelWidthTiles, panelHeightTiles, *ctx.tileConfig);

    // Text sits inside the frame's tile-wide border on every side.
    const int textX = startX + tileSize;
    const int textWidth = panelWidth - tileSize * 2;
    int textY = startY + tileSize;

    // Title, centred across the panel rather than on the text column.
    const std::string_view title = "* YOU HAVE DIED *";
    const int titleX = startX + (panelWidth - renderer.measure_text(title)) / 2;
    renderer.draw_text_color(Vector2D{ titleX, textY }, title, RED);
    textY += TEXT_ROW_PITCH * 2;

    renderer.draw_text_color(
        Vector2D{ textX, textY },
        std::format("{} {} - Level {}", playerRace, playerClass, playerLevel),
        WHITE);
    textY += TEXT_ROW_PITCH;

    renderer.draw_text_color(
        Vector2D{ textX, textY },
        std::format("Dungeon Level : {}", dungeonLevel),
        YELLOW);
    textY += TEXT_ROW_PITCH;

    renderer.draw_text_color(
        Vector2D{ textX, textY },
        std::format("Monsters Slain: {}", killCount),
        YELLOW);
    textY += TEXT_ROW_PITCH;

    renderer.draw_text_color(
        Vector2D{ textX, textY },
        std::format("Experience    : {}", playerXp),
        YELLOW);
    textY += TEXT_ROW_PITCH * 2;

    renderer.draw_text_color(Vector2D{ textX, textY }, "-- Last Messages --", GRAY);
    textY += TEXT_ROW_PITCH;

    // The prompt owns the bottom row, so the log stops before reaching it.
    const int promptY = startY + panelHeight - tileSize - renderer.get_font_size();
    for (const std::string& line : recentMessages)
    {
        if (textY + TEXT_ROW_PITCH > promptY)
        {
            break;
        }
        renderer.draw_text_color(
            Vector2D{ textX, textY },
            renderer.fit_text_to_width(line, textWidth),
            LIGHTGRAY);
        textY += TEXT_ROW_PITCH;
    }

    const std::string_view prompt = "[ ENTER ] Return to main menu";
    const int promptX = startX + (panelWidth - renderer.measure_text(prompt)) / 2;
    renderer.draw_text_color(Vector2D{ promptX, promptY }, prompt, Color{ 0, 255, 255, 255 });
}

void DeathMenu::handle_input(GameContext& ctx)
{
    if (lastKey == GameKey::ENTER)
    {
        run = false;
        ctx.menus->clear();
        ctx.menus->push_back(make_main_menu(true, ctx));
    }
}
