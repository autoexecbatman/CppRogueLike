#include "SpellAnimations.h"

#include <curses.h>
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

namespace
{
    struct ExplosionData
    {
        WINDOW* window{};
        int radius{};
        int fullDiameter{};
    };

    static const int FIRE_COLORS[] = {
        RED_YELLOW_PAIR,
        RED_YELLOW_PAIR,
        WHITE_RED_PAIR
    };

    static const char FIRE_CHARS[] = {
        '*', '#', '&', '@', '%', '+', '=', '^'
    };

    constexpr size_t FIRE_COLOR_COUNT = std::size(FIRE_COLORS);
    constexpr size_t FIRE_CHAR_COUNT  = std::size(FIRE_CHARS);

    double calculate_distance_from_center(int y, int x, int centerRadius)
    {
        return std::sqrt(std::pow(y - centerRadius, 2) + std::pow(x - centerRadius, 2));
    }

    bool is_within_explosion_radius(int y, int x, int centerRadius, int currentRadius)
    {
        double distance = calculate_distance_from_center(y, x, centerRadius);
        return distance <= currentRadius;
    }

    int get_intensity_based_color(double intensity, GameContext& ctx)
    {
        if (ctx.dice->roll(1, 100) <= static_cast<int>(intensity * 100))
        {
            return ctx.dice->roll(1, 2) - 1;
        }
        return 2;
    }

    int get_fade_color(GameContext& ctx)
    {
        if (ctx.dice->roll(1, 100) <= 30)
        {
            return ctx.dice->roll(1, static_cast<int>(FIRE_COLOR_COUNT)) - 1;
        }
        return 2;
    }

    void draw_fire_character(WINDOW* window, int y, int x, int colorIndex, GameContext& ctx)
    {
        if (colorIndex < 0 || colorIndex >= static_cast<int>(FIRE_COLOR_COUNT))
        {
            colorIndex = ctx.dice->roll(0, static_cast<int>(FIRE_COLOR_COUNT) - 1);
        }

        wattron(window, COLOR_PAIR(FIRE_COLORS[colorIndex]));
        mvwaddch(window, y, x,
            FIRE_CHARS[ctx.dice->roll(0, static_cast<int>(FIRE_CHAR_COUNT) - 1)]
        );
        wattroff(window, COLOR_PAIR(FIRE_COLORS[colorIndex]));
    }

    void draw_explosion_frame(const ExplosionData& explosion, int currentRadius, GameContext& ctx)
    {
        for (int y = 0; y < explosion.fullDiameter; y++)
        {
            for (int x = 0; x < explosion.fullDiameter; x++)
            {
                if (is_within_explosion_radius(y, x, explosion.radius, currentRadius))
                {
                    draw_fire_character(explosion.window, y, x, -1, ctx);
                }
            }
        }
    }

    void draw_flickering_explosion(const ExplosionData& explosion, GameContext& ctx)
    {
        for (int y = 0; y < explosion.fullDiameter; y++)
        {
            for (int x = 0; x < explosion.fullDiameter; x++)
            {
                double distance = calculate_distance_from_center(y, x, explosion.radius);

                if (distance <= explosion.radius)
                {
                    double intensity = 1.0 - (distance / explosion.radius);
                    int colorIndex = get_intensity_based_color(intensity, ctx);
                    draw_fire_character(explosion.window, y, x, colorIndex, ctx);
                }
            }
        }
    }

    void draw_fading_explosion(const ExplosionData& explosion, int currentRadius, GameContext& ctx)
    {
        for (int y = 0; y < explosion.fullDiameter; y++)
        {
            for (int x = 0; x < explosion.fullDiameter; x++)
            {
                if (is_within_explosion_radius(y, x, explosion.radius, currentRadius))
                {
                    int colorIndex = get_fade_color(ctx);
                    draw_fire_character(explosion.window, y, x, colorIndex, ctx);
                }
            }
        }
    }

    void animate_expansion_phase(const ExplosionData& explosion, GameContext& ctx)
    {
        const int EXPANSION_FRAMES = 10;

        for (int frame = 0; frame < EXPANSION_FRAMES; frame++)
        {
            wclear(explosion.window);
            int currentRadius = (explosion.radius * frame) / EXPANSION_FRAMES;

            draw_explosion_frame(explosion, currentRadius, ctx);
            wrefresh(explosion.window);
            napms(30);
        }
    }

    void animate_peak_phase(const ExplosionData& explosion, GameContext& ctx)
    {
        const int PEAK_FRAMES = 20;

        for (int frame = 0; frame < PEAK_FRAMES; frame++)
        {
            draw_flickering_explosion(explosion, ctx);
            wrefresh(explosion.window);
            napms(50);
        }
    }

    void animate_fade_phase(const ExplosionData& explosion, GameContext& ctx)
    {
        const int FADE_FRAMES = 15;

        for (int frame = 0; frame < FADE_FRAMES; frame++)
        {
            wclear(explosion.window);
            int currentRadius = explosion.radius - (explosion.radius * frame) / FADE_FRAMES;

            draw_fading_explosion(explosion, currentRadius, ctx);
            wrefresh(explosion.window);
            napms(40);
        }
    }

    void cleanup_explosion_window(WINDOW* window, GameContext& ctx)
    {
        nodelay(window, false);
        delwin(window);

        clear();
        refresh();
        ctx.rendering_manager->render(ctx);
        ctx.gui->gui_render(ctx);
        ctx.rendering_manager->force_screen_refresh();
    }
}

namespace SpellAnimations
{
    void animate_lightning(Vector2D from, Vector2D to, GameContext& ctx)
    {
        const std::vector<Vector2D> mainPath = Map::bresenham_line(from, to);

        const char LIGHTNING_CHARS[] = { '/', '\\', '|', '-', '*', '+' };
        const int CHAR_COUNT = 6;

        // Create lightning flash
        clear();
        ctx.rendering_manager->render(ctx);
        ctx.gui->gui_render(ctx);

        // Lightning phases
        const int FLASH_COUNT = 3;

        for (int flash = 0; flash < FLASH_COUNT; flash++)
        {
            // Generate jagged lightning path with branches
            std::vector<Vector2D> lightningPath = mainPath;

            // Add jitter to make the bolt jagged
            for (size_t i = 1; i < lightningPath.size() - 1; i++)
            {
                // Every few segments, add jitter
                if (i % 2 == 0)
                {
                    lightningPath[i].x += ctx.dice->roll(-1, 1);
                    lightningPath[i].y += ctx.dice->roll(-1, 1);
                }
            }

            // Draw lightning path
            attron(COLOR_PAIR(WHITE_BLUE_PAIR));
            for (const auto& pos : lightningPath)
            {
                // Choose random lightning character for jagged effect
                char symbol = LIGHTNING_CHARS[ctx.dice->roll(0, CHAR_COUNT - 1)];
                mvaddch(pos.y, pos.x, symbol);
            }

            // Add branch lightning (smaller offshoots)
            int branchCount = 2 + flash;  // More branches in later flashes
            std::uniform_int_distribution<> branchPosDist(0, static_cast<int>(lightningPath.size()) - 1);
            std::uniform_int_distribution<> branchLengthDist(2, 5);

            for (int b = 0; b < branchCount; b++)
            {
                // Choose a random point along the main path to branch from
                if (lightningPath.size() > 3)
                {
                    int branchPos = ctx.dice->roll(0, static_cast<int>(lightningPath.size()) - 1);
                    Vector2D branchStart = lightningPath[branchPos];

                    // Generate branch in a random direction
                    int branchLength = ctx.dice->roll(2, 5);
                    int dirX = ctx.dice->roll(-1, 1);
                    int dirY = ctx.dice->roll(-1, 1);

                    // Ensure we have a direction (not zero)
                    if (dirX == 0 && dirY == 0) dirX = 1;

                    Vector2D current = branchStart;
                    for (int i = 0; i < branchLength; i++)
                    {
                        current.x += dirX;
                        current.y += dirY;

                        // Add some randomness to branch path
                        if (i > 0 && i % 2 == 0)
                        {
                            current.x += ctx.dice->roll(-1, 1);
                            current.y += ctx.dice->roll(-1, 1);
                        }

                        if (ctx.map->is_in_bounds(current))
                        {
                            char symbol = LIGHTNING_CHARS[ctx.dice->roll(0, CHAR_COUNT - 1)];
                            mvaddch(current.y, current.x, symbol);
                        }
                    }
                }
            }

            // Create a bright flash at target position
            mvaddch(to.y, to.x, '@');

            attroff(COLOR_PAIR(WHITE_BLUE_PAIR));
            refresh();

            // Flash timing - quick for lightning
            napms(70);

            // Clear for next flash
            if (flash < FLASH_COUNT - 1) {
                clear();
                ctx.rendering_manager->render(ctx);
                ctx.gui->gui_render(ctx);
                napms(50); // Brief darkness between flashes
            }
        }

        // End with a final impact flash
        attron(COLOR_PAIR(WHITE_BLUE_PAIR));
        // Draw impact markers around target
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) {
                    mvaddch(to.y, to.x, '*');
                } else {
                    int x = to.x + dx;
                    int y = to.y + dy;
                    if (x >= 0 && y >= 0 && x < ctx.map->get_width() && y < ctx.map->get_height()) {
                        mvaddch(y, x, '.');
                    }
                }
            }
        }
        attroff(COLOR_PAIR(WHITE_BLUE_PAIR));
        refresh();
        napms(150);

        // Redraw game
        clear();
        ctx.rendering_manager->render(ctx);
        ctx.gui->gui_render(ctx);
        refresh();
    }

    void animate_explosion(Vector2D center, int radius, GameContext& ctx)
    {
        ExplosionData explosion{};
        explosion.radius = radius;
        explosion.fullDiameter = radius * 2 + 1;

        Vector2D topLeft{ center.y - radius, center.x - radius };

        explosion.window = newwin(
            explosion.fullDiameter, // height
            explosion.fullDiameter, // width
            topLeft.y,              // y position
            topLeft.x               // x position
        );

        nodelay(explosion.window, true);

        animate_expansion_phase(explosion, ctx);
        animate_peak_phase(explosion, ctx);
        animate_fade_phase(explosion, ctx);

        cleanup_explosion_window(explosion.window, ctx);
    }

    void animate_creature_hit(Vector2D position)
    {
        attron(COLOR_PAIR(RED_YELLOW_PAIR));
        mvaddch(position.y, position.x, '~');
        attroff(COLOR_PAIR(RED_YELLOW_PAIR));
        refresh();
        napms(200);
    }
}
