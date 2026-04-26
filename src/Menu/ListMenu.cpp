// file: ListMenu.cpp
#include <algorithm>
#include <cctype>

#include <raylib.h>

#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "ListMenu.h"

ListMenu::ListMenu(
    std::string title,
    std::vector<MenuEntry> entries,
    std::function<void(GameContext&)> onEscape,
    std::function<void(GameContext&)> onFrame,
    GameContext& ctx)
    : title{ std::move(title) }
    , entries{ std::move(entries) }
    , onEscape{ std::move(onEscape) }
    , onFrame{ std::move(onFrame) }
{
    size_t maxLabelLen{ 0 };
    for (const auto& entry : this->entries)
    {
        if (entry.label.size() > maxLabelLen)
        {
            maxLabelLen = entry.label.size();
        }
    }
    // Width: enough for the widest label (2 border + 2 padding) or the title (2 border).
    // Height: one row per entry plus 2 border rows.
    menuWidth = std::max({ this->title.size() + 2, maxLabelLen + 4, size_t{ 10 } });
    menuHeight = this->entries.size() + 2;

    int vcols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 60;
    int vrows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 34;
    int startX = (vcols - static_cast<int>(menuWidth)) / 2;
    int startY = (vrows - static_cast<int>(menuHeight)) / 2;
    menuStartX = static_cast<size_t>(startX < 0 ? 0 : startX);
    menuStartY = static_cast<size_t>(startY < 0 ? 0 : startY);
    menu_new(menuWidth, menuHeight, menuStartX, menuStartY, ctx);
}

void ListMenu::draw_entries()
{
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (cursorIndex == i)
        {
            menu_highlight_on();
        }
        menu_print(1, static_cast<int>(i) + 1, entries[i].label);
        if (cursorIndex == i)
        {
            menu_highlight_off();
        }
    }
}

void ListMenu::draw()
{
    menu_clear();
    menu_draw_box();
    menu_draw_title(title, YELLOW_BLACK_PAIR);
    draw_entries();
    menu_refresh();
}

void ListMenu::on_key(int key, GameContext& ctx)
{
    switch (key)
    {

    case 0x103: // KEY_UP
    case 'w':
    {
        cursorIndex = (cursorIndex + entries.size() - 1) % entries.size();
        break;
    }

    case 0x102: // KEY_DOWN
    case 's':
    {
        cursorIndex = (cursorIndex + 1) % entries.size();
        break;
    }

    case 10: // ENTER
    {
        menu_set_run_false();
        if (entries[cursorIndex].command)
        {
            entries[cursorIndex].command(ctx);
        }
        break;
    }

    case 27: // ESCAPE
    {
        menu_set_run_false();
        if (onEscape)
        {
            onEscape(ctx);
        }
        break;
    }

    default:
    {
        // Hotkey match — case-insensitive so 'M' and 'm' both work.
        for (auto& entry : entries)
        {
            if (entry.hotkey != 0 && std::tolower(key) == std::tolower(entry.hotkey))
            {
                menu_set_run_false();
                if (entry.command)
                {
                    entry.command(ctx);
                }
                return;
            }
        }
        break;
    }

    }
}

void ListMenu::menu(GameContext& ctx)
{
    if (onFrame)
    {
        onFrame(ctx);
    }
    menu_key_listen();

    // Hover -- update cursor only when the mouse actually moves.
    // Without the delta guard, a stationary mouse inside the menu area
    // resets cursorIndex every frame, making keyboard UP/DOWN invisible.
    ::Vector2 mouseDelta = GetMouseDelta();
    bool mouseMoved = (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f);
    if (mouseMoved && inputSystem && renderer)
    {
        int tileSize = renderer->get_tile_size();
        ::Vector2 rawMouse = GetMousePosition();
        int relRow = static_cast<int>(rawMouse.y) / tileSize - static_cast<int>(menuStartY) - 1;
        if (relRow >= 0 && relRow < static_cast<int>(entries.size()))
        {
            cursorIndex = static_cast<size_t>(relRow);
        }
    }

    draw();

    // Use inputSystem->get_key() instead of IsMouseButtonPressed() directly.
    // On Emscripten, poll() (called inside menu_key_listen()) consumes the
    // prev->curr transition. A second raw IsMouseButtonPressed() call in the
    // same frame sees prev=curr=1 and returns false.
    if (inputSystem && inputSystem->get_key() == GameKey::MOUSE_LEFT)
    {
        if (renderer)
        {
            int tileSize = renderer->get_tile_size();
            ::Vector2 rawMouse = GetMousePosition();
            int relRow = static_cast<int>(rawMouse.y) / tileSize
                - static_cast<int>(menuStartY) - 1;
            if (relRow >= 0 && relRow < static_cast<int>(entries.size()))
            {
                menu_set_run_false();
                if (entries[static_cast<size_t>(relRow)].command)
                {
                    entries[static_cast<size_t>(relRow)].command(ctx);
                }
                return;
            }
        }
        // Click outside menu bounds = ignore. Do not cancel or fire onEscape.
        // onEscape is reserved for the ESC key — a misclick outside the menu
        // should never trigger quit or close behaviour.
        return;
    }

    on_key(keyPress, ctx);
}

// end of file: ListMenu.cpp
