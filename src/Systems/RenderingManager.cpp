// RenderingManager.cpp - Handles all rendering and screen management

#include "RenderingManager.h"
#include "../Map/Map.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Actor.h"
#include "../Items/Items.h"
#include "../Actor/Container.h"
#include "../Gui/Gui.h"
#include "../Objects/Web.h"
#include <curses.h>

void RenderingManager::render_world(
    const Map& map,
    const Stairs& stairs,
    std::span<const std::unique_ptr<Object>> objects,
    const Container& container,
    std::span<const std::unique_ptr<Creature>> creatures,
    const Player& player
) const
{
    map.render();
    stairs.render();

    render_objects(objects);
    render_items(container.inv);
    render_creatures(creatures);
    player.render();
}

void RenderingManager::render_creatures(std::span<const std::unique_ptr<Creature>> creatures) const
{
    for (const auto& creature : creatures)
    {
        if (creature)
        {
            creature->render();
        }
    }
}

void RenderingManager::render_items(std::span<const std::unique_ptr<Item>> items) const
{
    for (const auto& item : items)
    {
        if (item)
        {
            item->render();
        }
    }
}

void RenderingManager::safe_screen_clear()
{
#ifdef EMSCRIPTEN
    // For Emscripten: Don't use clear(), just redraw
    // Note: restore_game_display will be called separately
#else
    // For native builds: Use normal clear
    clear();
    refresh();
#endif
}

void RenderingManager::force_screen_refresh() const
{
    refresh();
    doupdate();  // Force immediate update
}

void RenderingManager::restore_game_display() const
{
    // Force screen refresh for display restoration
    force_screen_refresh();
}

void RenderingManager::render_objects(std::span<const std::unique_ptr<Object>> objects) const
{
    // Render any objects (like webs)
    for (const auto& obj : objects)
    {
        if (obj)
        {
            obj->render();
        }
    }
}
