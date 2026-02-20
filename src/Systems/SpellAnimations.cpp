#include "SpellAnimations.h"

#include <cmath>
#include <memory>
#include <vector>

#include "../Core/GameContext.h"
#include "../Utils/Vector2D.h"
#include "../Colors/Colors.h"
#include "../Systems/RenderingManager.h"
#include "../Gui/Gui.h"
#include "../Map/Map.h"
#include "../Random/RandomDice.h"
#include "../Renderer/Renderer.h"

namespace SpellAnimations
{
    void animate_lightning(Vector2D from, Vector2D to, GameContext& ctx)
    {
        if (!ctx.renderer || !ctx.rendering_manager) return;

        auto path = Map::bresenham_line(from, to);
        if (path.size() < 2) return;

        int ts    = ctx.renderer->get_tile_size();
        int cam_x = ctx.renderer->get_camera_x();
        int cam_y = ctx.renderer->get_camera_y();

        static constexpr int    FLASH_COUNT    = 3;
        static constexpr double FLASH_DURATION = 0.10;
        static constexpr double GAP_DURATION   = 0.04;

        for (int flash = 0; flash < FLASH_COUNT && !WindowShouldClose(); ++flash)
        {
            double start = GetTime();
            while (GetTime() - start < FLASH_DURATION && !WindowShouldClose())
            {
                float t     = static_cast<float>((GetTime() - start) / FLASH_DURATION);
                auto  alpha = static_cast<unsigned char>((1.0f - t * t) * 255.0f);

                ctx.renderer->begin_frame();
                ctx.rendering_manager->render(ctx);

                for (int i = 1; i < static_cast<int>(path.size()); ++i)
                {
                    const auto& pos = path[i];
                    int sx = pos.x * ts - cam_x;
                    int sy = pos.y * ts - cam_y;
                    // Alternate: bright white on even steps, blue tint on odd
                    Color c = (i % 2 == 0)
                        ? Color{220, 240, 255, alpha}
                        : Color{100, 160, 255, static_cast<unsigned char>(alpha / 2)};
                    DrawRectangle(sx, sy, ts, ts, c);
                }

                // Impact flash centered on target
                {
                    int sx = to.x * ts - cam_x - ts / 2;
                    int sy = to.y * ts - cam_y - ts / 2;
                    DrawRectangle(
                        sx, sy, ts * 2, ts * 2,
                        Color{255, 255, 120, static_cast<unsigned char>(alpha * 3 / 5)}
                    );
                }

                ctx.renderer->end_frame();
            }

            // Brief gap between flashes (show map without bolt)
            if (flash < FLASH_COUNT - 1)
            {
                double gap = GetTime();
                while (GetTime() - gap < GAP_DURATION && !WindowShouldClose())
                {
                    ctx.renderer->begin_frame();
                    ctx.rendering_manager->render(ctx);
                    ctx.renderer->end_frame();
                }
            }
        }
    }

    void animate_explosion(Vector2D center, int radius, GameContext& ctx)
    {
        if (!ctx.renderer || !ctx.rendering_manager) return;

        int ts    = ctx.renderer->get_tile_size();
        int cam_x = ctx.renderer->get_camera_x();
        int cam_y = ctx.renderer->get_camera_y();
        int cx_px = center.x * ts - cam_x + ts / 2;
        int cy_px = center.y * ts - cam_y + ts / 2;

        static constexpr int    EXPAND_STEPS  = 4;
        static constexpr double STEP_DURATION = 0.07;
        static constexpr double FADE_DURATION = 0.15;

        // Expanding ring phases
        for (int step = 1; step <= EXPAND_STEPS && !WindowShouldClose(); ++step)
        {
            float expand = static_cast<float>(step) / EXPAND_STEPS;
            float ring_r = expand * static_cast<float>(radius * ts);

            double start = GetTime();
            while (GetTime() - start < STEP_DURATION && !WindowShouldClose())
            {
                float t     = static_cast<float>((GetTime() - start) / STEP_DURATION);
                auto  alpha = static_cast<unsigned char>((1.0f - t) * 220.0f);

                ctx.renderer->begin_frame();
                ctx.rendering_manager->render(ctx);

                DrawCircle(
                    cx_px, cy_px, ring_r,
                    Color{255, 140, 20, static_cast<unsigned char>(alpha / 4)}
                );
                DrawCircleLines(cx_px, cy_px, ring_r, Color{255, 200, 50, alpha});

                ctx.renderer->end_frame();
            }
        }

        // Fade-out at full radius
        {
            float full_r = static_cast<float>(radius * ts);
            double start = GetTime();
            while (GetTime() - start < FADE_DURATION && !WindowShouldClose())
            {
                float t     = static_cast<float>((GetTime() - start) / FADE_DURATION);
                auto  alpha = static_cast<unsigned char>((1.0f - t) * 140.0f);

                ctx.renderer->begin_frame();
                ctx.rendering_manager->render(ctx);

                DrawCircle(
                    cx_px, cy_px, full_r,
                    Color{255, 80, 0, static_cast<unsigned char>(alpha / 3)}
                );
                DrawCircleLines(cx_px, cy_px, full_r, Color{200, 80, 0, alpha});

                ctx.renderer->end_frame();
            }
        }
    }

    void animate_creature_hit(Vector2D position, GameContext& ctx)
    {
        if (!ctx.renderer || !ctx.rendering_manager) return;

        int ts = ctx.renderer->get_tile_size();
        int sx = position.x * ts - ctx.renderer->get_camera_x();
        int sy = position.y * ts - ctx.renderer->get_camera_y();

        static constexpr double DURATION = 0.28;
        static constexpr double FLASH_HZ = 10.0;

        double start = GetTime();
        while (GetTime() - start < DURATION && !WindowShouldClose())
        {
            double elapsed = GetTime() - start;
            float  t       = static_cast<float>(elapsed / DURATION);
            float  pulse   = std::sin(
                static_cast<float>(elapsed) * 6.28318f * static_cast<float>(FLASH_HZ)
            );
            auto alpha = static_cast<unsigned char>((1.0f - t) * (140.0f + 50.0f * pulse));

            ctx.renderer->begin_frame();
            ctx.rendering_manager->render(ctx);
            DrawRectangle(sx, sy, ts, ts, Color{255, 40, 40, alpha});
            ctx.renderer->end_frame();
        }
    }
}
