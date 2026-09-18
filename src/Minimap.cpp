// file: Map/Minimap.cpp
#include <algorithm>

#include <raylib.h>

#include "Stairs.h"
#include "Player.h"
#include "GameContext.h"
#include "TileConfig.h"
#include "Renderer.h"
#include "Map.h"
#include "Minimap.h"

void Minimap::toggle() noexcept
{
    visible = !visible;
}

bool Minimap::is_visible() const noexcept
{
    return visible;
}

void Minimap::render(const GameContext& ctx) const
{
    if (!visible || !ctx.map || !ctx.renderer || !ctx.player())
    {
        return;
    }

    const Map& map = *ctx.map;
    const Renderer& renderer = *ctx.renderer;

    int mapW = map.get_width();
    int mapH = map.get_height();
    int screenW = renderer.get_screen_width();
    int screenH = renderer.get_screen_height();

    // The largest whole number of pixels a map tile can take and still leave the
    // overlay inside its share of the window. Whole pixels, so no cell is a
    // different size from its neighbour.
    const int tilePx = std::max(1, std::min(
        screenW * MAX_WIDTH_PERCENT / 100 / mapW,
        screenH * MAX_HEIGHT_PERCENT / 100 / mapH));

    int panelW = mapW * tilePx;
    int panelH = mapH * tilePx;
    int originX = screenW - panelW - PADDING;
    int originY = PADDING;

    // Opaque, with an edge. A translucent black backdrop is invisible against the
    // unexplored parts of the map, which left the overlay reading as loose specks
    // with no bounds.
    DrawRectangle(originX - BORDER, originY - BORDER, panelW + 2 * BORDER, panelH + 2 * BORDER, Color{ 20, 12, 28, 255 });
    DrawRectangleLines(originX - BORDER, originY - BORDER, panelW + 2 * BORDER, panelH + 2 * BORDER, Color{ 120, 120, 150, 255 });

    for (int y = 0; y < mapH; ++y)
    {
        for (int x = 0; x < mapW; ++x)
        {
            Vector2D pos{ x, y };
            if (!map.is_explored(pos))
            {
                continue;
            }

            bool inFov = map.is_in_fov(pos);
            Color c{};

            const TileDefinition& definition = ctx.tileConfig->get_tile_definition(map.get_tile_type(pos));
            const TileColor& authored = inFov ? definition.minimapVisible : definition.minimapRemembered;
            c = Color{
                static_cast<unsigned char>(authored.red),
                static_cast<unsigned char>(authored.green),
                static_cast<unsigned char>(authored.blue),
                static_cast<unsigned char>(authored.alpha)
            };

            DrawRectangle(originX + x * tilePx, originY + y * tilePx, tilePx, tilePx, c);
        }
    }

    if (ctx.stairs)
    {
        Vector2D sp = ctx.stairs->position;
        if (map.is_explored(sp))
        {
            DrawRectangle(
                originX + sp.x * tilePx - 1,
                originY + sp.y * tilePx - 1,
                tilePx + 2,
                tilePx + 2,
                Color{ 255, 210, 50, 255 });
        }
    }

    Vector2D pp = ctx.player()->position;
    DrawRectangle(
        originX + pp.x * tilePx - 1,
        originY + pp.y * tilePx - 1,
        tilePx + 2,
        tilePx + 2,
        Color{ 255, 255, 0, 255 });
}
