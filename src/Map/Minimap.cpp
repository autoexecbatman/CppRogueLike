// file: Map/Minimap.cpp
#include <raylib.h>

#include "../Actor/Stairs.h"
#include "../ActorTypes/Player.h"
#include "../Core/GameContext.h"
#include "../Systems/TileConfig.h"
#include "../Renderer/Renderer.h"
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

    int panelW = mapW * TILE_PX;
    int panelH = mapH * TILE_PX;
    int originX = screenW - panelW - PADDING;
    int originY = PADDING;

    DrawRectangle(originX - 2, originY - 2, panelW + 4, panelH + 4, Color{ 0, 0, 0, 200 });

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

            DrawRectangle(originX + x * TILE_PX, originY + y * TILE_PX, TILE_PX, TILE_PX, c);
        }
    }

    if (ctx.stairs)
    {
        Vector2D sp = ctx.stairs->position;
        if (map.is_explored(sp))
        {
            DrawRectangle(
                originX + sp.x * TILE_PX - 1,
                originY + sp.y * TILE_PX - 1,
                TILE_PX + 2,
                TILE_PX + 2,
                Color{ 255, 210, 50, 255 });
        }
    }

    Vector2D pp = ctx.player()->position;
    DrawRectangle(
        originX + pp.x * TILE_PX - 1,
        originY + pp.y * TILE_PX - 1,
        TILE_PX + 2,
        TILE_PX + 2,
        Color{ 255, 255, 0, 255 });
}
