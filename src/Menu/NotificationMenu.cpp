// file: NotificationMenu.cpp
#include <algorithm>
#include <format>

#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "NotificationMenu.h"

NotificationMenu::NotificationMenu(
    std::string title,
    std::vector<std::string> lines,
    GameContext& ctx)
    : title{ std::move(title) }
    , lines{ std::move(lines) }
{
    size_t maxLineLen{ 0 };
    for (const auto& line : this->lines)
    {
        if (line.size() > maxLineLen)
        {
            maxLineLen = line.size();
        }
    }

    // Width: fits widest line (2 border + 2 padding) or title (2 border).
    // Height: 1 top border + N lines + 1 prompt row + 1 bottom border.
    static constexpr size_t minWidth{ 10 };
    menuWidth = std::max({ this->title.size() + 2, maxLineLen + 4, minWidth });
    menuHeight = this->lines.size() + 3;

    int vcols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 60;
    int vrows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 34;
    int startX = (vcols - static_cast<int>(menuWidth)) / 2;
    int startY = (vrows - static_cast<int>(menuHeight)) / 2;
    menuStartX = static_cast<size_t>(startX < 0 ? 0 : startX);
    menuStartY = static_cast<size_t>(startY < 0 ? 0 : startY);
    menu_new(menuWidth, menuHeight, menuStartX, menuStartY, ctx);
}

NotificationMenu::~NotificationMenu()
{
    menu_delete();
}

void NotificationMenu::draw()
{
    menu_clear();
    menu_draw_box();
    menu_draw_title(title, YELLOW_BLACK_PAIR);

    int row{ 1 };
    for (const auto& line : lines)
    {
        menu_print(2, row, line);
        ++row;
    }

    // Centered prompt — gives the player a visual cue to press any key.
    int promptCol = (static_cast<int>(menuWidth) - 16) / 2;
    menu_print(promptCol < 1 ? 1 : promptCol, row, "[ press any key ]");

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
    if (keyPress != 0)
    {
        menu_set_run_false();
        if (onClose)
        {
            onClose(ctx);
        }
    }
}
