// file: Systems/TargetingMenu.cpp
#include <cmath>

#include <raylib.h>

#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Map/Map.h"
#include "../Renderer/InputSystem.h"
#include "../Renderer/Renderer.h"
#include "../Systems/RenderingManager.h"
#include "../Systems/TargetingSystem.h"
#include "../Utils/Vector2D.h"
#include "TargetingMenu.h"

TargetingMenu::TargetingMenu(int maxRange, int aoeRadius, Callback onComplete, GameContext& ctx)
    : cursor(ctx.player->position)
    , maxRange(maxRange)
    , aoeRadius(aoeRadius)
    , onComplete(std::move(onComplete))
{
}

void TargetingMenu::menu(GameContext& ctx)
{
    ctx.inputSystem->poll();
    GameKey key = ctx.inputSystem->get_key();

    Vector2D move{ 0, 0 };
    switch (key)
    {

    case GameKey::UP:
    case GameKey::W:
    {
        move = DIR_N;
        break;
    }

    case GameKey::DOWN:
    case GameKey::S:
    {
        move = DIR_S;
        break;
    }

    case GameKey::LEFT:
    case GameKey::A:
    {
        move = DIR_W;
        break;
    }

    case GameKey::RIGHT:
    case GameKey::D:
    {
        move = DIR_E;
        break;
    }

    case GameKey::Q:
    {
        move = DIR_NW;
        break;
    }

    case GameKey::E:
    {
        move = DIR_NE;
        break;
    }

    case GameKey::Z:
    {
        move = DIR_SW;
        break;
    }

    case GameKey::C:
    {
        move = DIR_SE;
        break;
    }

    case GameKey::ENTER:
    {
        menu_set_run_false();
        onComplete(ctx.map->is_in_bounds(cursor), cursor, ctx);
        return;
    }

    case GameKey::ESCAPE:
    {
        menu_set_run_false();
        onComplete(false, cursor, ctx);
        return;
    }

    default:
        break;
    }

    if (move.x != 0 || move.y != 0)
    {
        Vector2D next = cursor + move;
        bool inBounds = ctx.map->is_in_bounds(next);
        bool inRange = maxRange <= 0 || next.distance_to(ctx.player->position) <= static_cast<double>(maxRange);
        if (inBounds && inRange)
        {
            cursor = next;
        }
    }

    ctx.renderer->begin_frame();
    ctx.renderingManager->render(ctx);

    ctx.targeting->draw_range_indicator(ctx, ctx.player->position, maxRange);
    ctx.targeting->draw_los(ctx, cursor);
    if (aoeRadius > 0)
    {
        ctx.targeting->draw_aoe_preview(ctx, cursor, aoeRadius);
    }

    draw_cursor(ctx);

    ctx.renderer->draw_text(
        Vector2D{ 4, 4 },
        "Select target -- arrows/WASD: move  Enter: confirm  Esc: cancel",
        WHITE_BLACK_PAIR);

    ctx.renderer->end_frame();
}

void TargetingMenu::draw_cursor(GameContext& ctx) const
{
    int tileSize = ctx.renderer->get_tile_size();
    int sx = cursor.x * tileSize - ctx.renderer->get_camera_x();
    int sy = cursor.y * tileSize - ctx.renderer->get_camera_y();
    float pulse = (std::sin(static_cast<float>(GetTime()) * 6.28318f * 4.0f) + 1.0f) * 0.5f;
    auto fillAlpha = static_cast<unsigned char>(40.0f + 30.0f * pulse);
    auto ringAlpha = static_cast<unsigned char>(120.0f + 100.0f * pulse);

    DrawRectangle(sx, sy, tileSize, tileSize, Color{ 255, 255, 50, fillAlpha });
    DrawRectangleLinesEx(
        Rectangle{
            static_cast<float>(sx),
            static_cast<float>(sy),
            static_cast<float>(tileSize),
            static_cast<float>(tileSize) },
        2.0f,
        Color{ 255, 255, 50, ringAlpha });
}

// end of file: Systems/TargetingMenu.cpp
