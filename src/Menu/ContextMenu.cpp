#include <algorithm>
#include <string>

#include <raylib.h>

#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/RenderingManager.h"
#include "ContextMenu.h"

ContextMenu::ContextMenu(std::vector<std::string> options, int anchor_col, int anchor_row, GameContext& ctx)
    : menuOptions(std::move(options))
{
    size_t longest = 0;
    for (const auto& opt : menuOptions)
    {
        longest = std::max(longest, opt.size());
    }

    int height = static_cast<int>(menuOptions.size()) + 3;
    int width = std::max(static_cast<int>(longest) + 4, 14);

    int viewport_cols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 80;
    int viewport_rows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 24;

    int starty = std::clamp(anchor_row, 0, std::max(0, viewport_rows - height));
    int startx = std::clamp(anchor_col, 0, std::max(0, viewport_cols - width));

    menu_new(
        static_cast<size_t>(height),
        static_cast<size_t>(width),
        static_cast<size_t>(starty),
        static_cast<size_t>(startx),
        ctx);
}

void ContextMenu::draw_content()
{
    menu_draw_box();
    menu_draw_title("Action", WHITE_BLACK_PAIR);

    for (int i = 0; i < static_cast<int>(menuOptions.size()); ++i)
    {
        if (i == selectedIndex)
        {
            menu_highlight_on();
        }
        menu_print(2, i + 2, menuOptions[static_cast<size_t>(i)]);
        if (i == selectedIndex)
        {
            menu_highlight_off();
        }
    }
}

void ContextMenu::menu(GameContext& ctx)
{
    int maxIndex = static_cast<int>(menuOptions.size()) - 1;

    while (run)
    {
        if (ctx.renderingManager)
        {
            ctx.renderingManager->render(ctx);
        }
        draw_content();
        menu_refresh();
        menu_key_listen();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            int tileSize = renderer ? renderer->get_tile_size() : 16;
            Vector2D mousePos = input_system->get_mouse_tile(tileSize);
            int relRow = static_cast<int>(mousePos.y) - static_cast<int>(menu_starty);
            if (relRow >= 2 && relRow < 2 + static_cast<int>(menuOptions.size()))
            {
                selected = relRow - 2;
                run = false;
                continue;
            }
            selected = -1;
            run = false;
            continue;
        }

        switch (keyPress)
        {
        case 0x103: // UP
        {
            if (selectedIndex > 0)
            {
                selectedIndex--;
            }
            break;
        }
        case 0x102: // DOWN
        {
            if (selectedIndex < maxIndex)
            {
                selectedIndex++;
            }
            break;
        }
        case '\n':
        case ' ':
        {
            selected = selectedIndex;
            run = false;
            break;
        }
        case 27: // ESC
        {
            selected = -1;
            run = false;
            break;
        }
        default:
            break;
        }
    }
}
