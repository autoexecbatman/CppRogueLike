// File: Fireball.cpp
#include <cmath>

#include "Fireball.h"
#include "LightningBolt.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/TargetingSystem.h"
#include "../Systems/CreatureManager.h"
#include "../Systems/RenderingManager.h"
#include "../Gui/Gui.h"
#include "../ActorTypes/Player.h"

//==Fireball==
Fireball::Fireball(int range, int damage) : LightningBolt(range, damage) {}

bool Fireball::use(Item& owner, Creature& wearer, GameContext& ctx)
{
    Vector2D tilePicked{ 0, 0 };
    if (!ctx.targeting_system->pick_tile(ctx, &tilePicked, Fireball::maxRange))
    {
        return false;
    }

    // burn everything in <range> (including player)
    ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format("The fireball explodes, burning everything within {} tiles!", Fireball::maxRange));
    ctx.message_system->finalize_message();

    // Create a more dynamic, realistic fire explosion
    create_explosion(tilePicked, ctx);

    // if the player is in range of the fireball animate the player
    if (ctx.player->get_tile_distance(tilePicked) <= Fireball::maxRange)
    {
        animation(ctx.player->position, maxRange);
        // damage the player
        ctx.player->destructible->take_damage(*ctx.player, damage, ctx);
    }

    // First pass to show affected creatures and display messages
    for (const auto& c : *ctx.creatures)
    {
        if (c)
        {
            if (!c->destructible->is_dead() && c->get_tile_distance(tilePicked) <= Fireball::maxRange)
            {
                ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format("The {} gets engulfed in flames!", c->actorData.name));
                ctx.message_system->append_message_part(WHITE_BLACK_PAIR, std::format(" ({} damage)", damage));
                ctx.message_system->finalize_message();
                animation(c->position, maxRange);
            }
        }
    }

    // Second pass to actually apply damage (separated to prevent issues if creatures die)
    for (const auto& c : *ctx.creatures)
    {
        if (c)
        {
            if (!c->destructible->is_dead() && c->get_tile_distance(tilePicked) <= Fireball::maxRange)
            {
                c->destructible->take_damage(*c, damage, ctx);
            }
        }
    }

    // CRITICAL FIX: Immediately cleanup dead creatures to show corpses
    ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);

    return Pickable::use(owner, wearer, ctx);
}

void Fireball::create_explosion(Vector2D center, GameContext& ctx)
{
    ExplosionData explosion = initialize_explosion_data(center);

    animate_expansion_phase(explosion, ctx);
    animate_peak_phase(explosion, ctx);
    animate_fade_phase(explosion, ctx);

    cleanup_explosion_window(explosion.window, ctx);
}

ExplosionData Fireball::initialize_explosion_data(Vector2D center)
{
    ExplosionData data{};
    data.radius = Fireball::maxRange;
    data.fullDiameter = data.radius * 2 + 1;
    
    Vector2D topLeft{ center.y - data.radius, center.x - data.radius };
    
    data.window = newwin(
        data.fullDiameter, // height
        data.fullDiameter, // width
        topLeft.y,         // y position
        topLeft.x          // x position
    );
    
    nodelay(data.window, true);
    return data;
}

void Fireball::animate_expansion_phase(const ExplosionData& explosion, GameContext& ctx)
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

void Fireball::animate_peak_phase(const ExplosionData& explosion, GameContext& ctx)
{
    const int PEAK_FRAMES = 20;

    for (int frame = 0; frame < PEAK_FRAMES; frame++)
    {
        draw_flickering_explosion(explosion, ctx);
        wrefresh(explosion.window);
        napms(50);
    }
}

void Fireball::animate_fade_phase(const ExplosionData& explosion, GameContext& ctx)
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

void Fireball::draw_explosion_frame(const ExplosionData& explosion, int currentRadius, GameContext& ctx)
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

void Fireball::draw_flickering_explosion(const ExplosionData& explosion, GameContext& ctx)
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

void Fireball::draw_fading_explosion(const ExplosionData& explosion, int currentRadius, GameContext& ctx)
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

bool Fireball::is_within_explosion_radius(int y, int x, int centerRadius, int currentRadius)
{
    double distance = calculate_distance_from_center(y, x, centerRadius);
    return distance <= currentRadius;
}

double Fireball::calculate_distance_from_center(int y, int x, int centerRadius)
{
    return std::sqrt(std::pow(y - centerRadius, 2) + std::pow(x - centerRadius, 2));
}

int Fireball::get_intensity_based_color(double intensity, GameContext& ctx)
{
    if (ctx.dice_roller->roll(1, 100) <= static_cast<int>(intensity * 100))
    {
        return ctx.dice_roller->roll(1, 2) - 1;  // More intense colors (0 or 1)
    }
    return 2;  // Less intense color
}

int Fireball::get_fade_color(GameContext& ctx)
{
    if (ctx.dice_roller->roll(1, 100) <= 30)
    {
        return ctx.dice_roller->roll(1, FIRE_COLOR_COUNT) - 1;
    }
    return 2;  // Least intense color predominates
}

void Fireball::draw_fire_character(WINDOW* window, int y, int x, int colorIndex, GameContext& ctx)
{
    static const int FIRE_COLORS[] = {
        RED_YELLOW_PAIR,
        RED_YELLOW_PAIR,
        WHITE_RED_PAIR
    };

    static const char FIRE_CHARS[] = {
        '*', '#', '&', '@', '%', '+', '=', '^'
    };

    constexpr int FIRE_COLOR_COUNT = std::size(FIRE_COLORS);
    constexpr int FIRE_CHAR_COUNT  = std::size(FIRE_CHARS);

    if (colorIndex < 0 || colorIndex >= FIRE_COLOR_COUNT)
    {
        colorIndex = ctx.dice_roller->roll(0, FIRE_COLOR_COUNT - 1);
    }

    wattron(window, COLOR_PAIR(FIRE_COLORS[colorIndex]));
    mvwaddch(window, y, x,
        FIRE_CHARS[ctx.dice_roller->roll(0, FIRE_CHAR_COUNT - 1)]
    );
    wattroff(window, COLOR_PAIR(FIRE_COLORS[colorIndex]));
}

void Fireball::cleanup_explosion_window(WINDOW* window, GameContext& ctx)
{
    nodelay(window, false);
    delwin(window);

    clear();
    refresh();
    ctx.rendering_manager->render(ctx);
    ctx.gui->gui_render(ctx);
    ctx.rendering_manager->force_screen_refresh();
}

void Fireball::animation(Vector2D position, int maxRange)
{
	// Simple animation: just display '~' character at creature position
	attron(COLOR_PAIR(RED_YELLOW_PAIR));
	mvaddch(position.y, position.x, '~');
	attroff(COLOR_PAIR(RED_YELLOW_PAIR));
	refresh();
	
	// Brief pause to show the effect
	napms(200);
}

void Fireball::load(const json& j)
{
    maxRange = j["maxRange"].get<int>();
    damage = j["damage"].get<int>();
}

void Fireball::save(json& j)
{
    j["type"] = static_cast<int>(PickableType::FIREBALL);
    j["maxRange"] = maxRange;
    j["damage"] = damage;
}