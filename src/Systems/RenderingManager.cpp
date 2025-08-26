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
    Map& map,
    Stairs& stairs,
    std::span<std::unique_ptr<Object>> objects,
    Container& container,
    std::span<std::unique_ptr<Creature>> creatures,
    Player& player
)
{
    map.render();
    stairs.render();

    render_objects(objects);
    render_items(container.inv);
    render_creatures(creatures);
    player.render();
}

void RenderingManager::render_creatures(std::span<std::unique_ptr<Creature>> creatures) const
{
    for (const auto& creature : creatures)
    {
        if (creature)
        {
            creature->render();
        }
    }
}

void RenderingManager::render_items(std::span<std::unique_ptr<Item>> items) const
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

void RenderingManager::restore_game_display(Map& map, Gui& gui)
{
    // This method coordinates the restoration - actual rendering delegated appropriately
    // The full game render and gui render will be called from Game class
    force_screen_refresh();
}

void RenderingManager::render_objects(std::span<std::unique_ptr<Object>> objects) const
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
