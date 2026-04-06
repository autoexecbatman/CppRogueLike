// file: Tools/BalanceViewer.cpp
#include <format>
#include <string>

#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "BalanceViewer.h"

BalanceViewer::BalanceViewer(int dungeonLevel, GameContext& ctx)
    : dungeonLevel(dungeonLevel)
{
    monsterDist = ctx.map->get_monster_distribution(dungeonLevel);
    itemDist = ctx.map->get_item_distribution(dungeonLevel);
}

void BalanceViewer::menu(GameContext& ctx)
{
    ctx.inputSystem->poll();
    GameKey key = ctx.inputSystem->get_key();
    if (key == GameKey::ESCAPE || key == GameKey::SPACE)
    {
        menu_set_run_false();
        return;
    }

    ctx.renderer->begin_frame();

    int tileSize = ctx.renderer->get_tile_size();
    int fontOff = (tileSize - ctx.renderer->get_font_size()) / 2;
    int vcols = ctx.renderer->get_viewport_cols();
    int vrows = ctx.renderer->get_viewport_rows();

    ctx.renderer->draw_frame(Vector2D{ 0, 0 }, vcols, vrows, *ctx.tileConfig);

    std::string title = std::format("BALANCE VIEWER  --  Dungeon Level {}", dungeonLevel);
    int titleW = ctx.renderer->measure_text(title);
    int titleX = (vcols * tileSize - titleW) / 2;
    ctx.renderer->draw_text(Vector2D{ titleX, fontOff }, title, YELLOW_BLACK_PAIR);

    std::string_view hint = "[ESC] or [SPACE] to close";
    int hintW = ctx.renderer->measure_text(hint);
    int hintX = (vcols * tileSize - hintW) / 2;
    ctx.renderer->draw_text(Vector2D{ hintX, (vrows - 1) * tileSize + fontOff }, hint, CYAN_BLACK_PAIR);

    // Left column: monsters. Right column: items.
    const int halfCols = vcols / 2;
    draw_column("MONSTERS", monsterDist, 0, ctx);
    draw_item_column("ITEMS", itemDist, halfCols, ctx);

    ctx.renderer->end_frame();
}

void BalanceViewer::draw_column(
    std::string_view header,
    const std::vector<MonsterPercentage>& dist,
    int startCol,
    GameContext& ctx)
{
    int tileSize = ctx.renderer->get_tile_size();
    int fontOff = (tileSize - ctx.renderer->get_font_size()) / 2;
    int vrows = ctx.renderer->get_viewport_rows();

    int x = startCol * tileSize + tileSize;
    int row = 2;

    ctx.renderer->draw_text(Vector2D{ x, row * tileSize + fontOff }, header, GREEN_BLACK_PAIR);
    ++row;

    for (const auto& entry : dist)
    {
        if (row >= vrows - 1)
        {
            break;
        }

        std::string line = std::format("{}  {:.1f}%", entry.name, entry.percentage);
        ctx.renderer->draw_text(Vector2D{ x, row * tileSize + fontOff }, line, WHITE_BLACK_PAIR);
        ++row;
    }
}

void BalanceViewer::draw_item_column(
    std::string_view header,
    const std::vector<ItemPercentage>& dist,
    int startCol,
    GameContext& ctx)
{
    int tileSize = ctx.renderer->get_tile_size();
    int fontOff = (tileSize - ctx.renderer->get_font_size()) / 2;
    int vrows = ctx.renderer->get_viewport_rows();

    int x = startCol * tileSize + tileSize;
    int row = 2;

    ctx.renderer->draw_text(Vector2D{ x, row * tileSize + fontOff }, header, GREEN_BLACK_PAIR);
    ++row;

    for (const auto& entry : dist)
    {
        if (row >= vrows - 1)
        {
            break;
        }

        std::string line = std::format("{:<24} {:5.1f}%  {}", entry.name, entry.percentage, entry.category);
        ctx.renderer->draw_text(Vector2D{ x, row * tileSize + fontOff }, line, WHITE_BLACK_PAIR);
        ++row;
    }
}

// end of file: Tools/BalanceViewer.cpp
