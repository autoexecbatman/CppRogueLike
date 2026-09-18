// file: NotificationMenu.cpp
#include <algorithm>
#include <cassert>
#include <format>

#include "Colors.h"
#include "GameContext.h"
#include "Renderer.h"
#include "NotificationMenu.h"

// The row every notification ends with, and part of what sets the menu's width.
static constexpr std::string_view PROMPT{ "[ press any key ]" };

NotificationMenu::NotificationMenu(
    std::string title,
    std::vector<std::string> lines,
    GameContext& ctx)
    : title{ std::move(title) }
    , lines{ std::move(lines) }
{
    assert(ctx.renderer && "NotificationMenu: renderer required before construction");
    const int tileSize = ctx.renderer->get_tile_size();
    const int fontSize = ctx.renderer->get_font_size();

    // Sized from measured text. A character count used as a tile count made the
    // help screen 37 tiles wide - 2368 pixels on a 1280-pixel screen - which threw
    // its centred title off to the right and filled the window top to bottom.
    int widestText = ctx.renderer->measure_text(this->title);
    for (const auto& line : this->lines)
    {
        widestText = std::max(widestText, ctx.renderer->measure_text(line));
    }
    widestText = std::max(widestText, ctx.renderer->measure_text(PROMPT));

    // One row per line, then the prompt row.
    const int rowCount = static_cast<int>(this->lines.size()) + 1;
    menuWidth = static_cast<size_t>(panel_tiles_for_text_width(widestText, tileSize));
    menuHeight = static_cast<size_t>(panel_tiles_for_text_rows(rowCount, tileSize, fontSize));

    int vcols = ctx.renderer->get_viewport_cols();
    int vrows = ctx.renderer->get_viewport_rows();
    int startX = (vcols - static_cast<int>(menuWidth)) / 2;
    int startY = (vrows - static_cast<int>(menuHeight)) / 2;
    menuStartX = static_cast<size_t>(startX < 0 ? 0 : startX);
    menuStartY = static_cast<size_t>(startY < 0 ? 0 : startY);
    menu_new(menuWidth, menuHeight, menuStartX, menuStartY, ctx);
}

void NotificationMenu::draw()
{
    menu_clear();
    menu_draw_box();
    menu_draw_title(title, YELLOW_BLACK_PAIR);

    int row{ 0 };
    for (const auto& line : lines)
    {
        menu_print(1, row, line);
        ++row;
    }

    // Centered prompt — gives the player a visual cue to press any key.
    menu_print_centered(row, std::string(PROMPT));

    menu_refresh();
}

void NotificationMenu::set_on_close(std::function<void(GameContext&)> callback)
{
    onClose = std::move(callback);
}

void NotificationMenu::menu(GameContext& ctx)
{
    menu_key_listen();
    draw();
    if (lastKey != GameKey::NONE || lastChar != 0)
    {
        menu_set_run_false();
        if (onClose)
        {
            onClose(ctx);
        }
    }
}
