// RenderingManager.cpp - Handles all rendering and screen management
#include "RenderingManager.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Map/Map.h"
#include "../ActorTypes/Player.h"
#include "../Actor/Actor.h"
#include "../Items/Items.h"
#include "../Actor/InventoryData.h"
#include "../Gui/Gui.h"
#include "../Objects/Web.h"

void RenderingManager::render(GameContext& ctx) const
{
    render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player, ctx);
}

void RenderingManager::render_world(
    const Map& map,
    const Stairs& stairs,
    std::span<const std::unique_ptr<Object>> objects,
    const InventoryData& inventory_data,
    std::span<const std::unique_ptr<Creature>> creatures,
    const Player& player,
    const GameContext& ctx
) const
{
    map.render(ctx);
    stairs.render(ctx);

    render_objects(objects, ctx);

    // Render floor items
    render_items(inventory_data.items, ctx);

    render_creatures(creatures, ctx);
    player.render(ctx);
}

void RenderingManager::render_creatures(std::span<const std::unique_ptr<Creature>> creatures, const GameContext& ctx) const
{
    for (const auto& creature : creatures)
    {
        if (creature)
        {
            creature->render(ctx);
        }
    }
}

void RenderingManager::render_items(std::span<const std::unique_ptr<Item>> items, const GameContext& ctx) const
{
    for (const auto& item : items)
    {
        if (item)
        {
            item->render(ctx);
        }
    }
}

void RenderingManager::safe_screen_clear()
{
    // No-op: Renderer handles frame clearing via ClearBackground(BLACK) in begin_frame()
}

void RenderingManager::force_screen_refresh() const
{
    // No-op: frame-based rendering handles this via Renderer::end_frame()
}

void RenderingManager::restore_game_display() const
{
    // Force screen refresh for display restoration
    force_screen_refresh();
}

void RenderingManager::restore_screen(GameContext& ctx) const
{
    render(ctx);
    ctx.gui->gui_render(ctx);
}

void RenderingManager::render_objects(std::span<const std::unique_ptr<Object>> objects, const GameContext& ctx) const
{
    // Render any objects (like webs)
    for (const auto& obj : objects)
    {
        if (obj)
        {
            obj->render(ctx);
        }
    }
}
