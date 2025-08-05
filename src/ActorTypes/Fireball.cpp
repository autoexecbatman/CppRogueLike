// File: Fireball.cpp
#include "Fireball.h"
#include "LightningBolt.h"
#include "../Actor/Actor.h"
#include "../Game.h"
#include "../Colors/Colors.h"
#include <cmath>
#include <random>

//==Fireball==
Fireball::Fireball(int range, int damage) : LightningBolt(range, damage) {}

bool Fireball::use(Item& owner, Creature& wearer)
{
    Vector2D tilePicked{ 0, 0 };

    if (!game.pick_tile(&tilePicked, Fireball::maxRange)) // <-- runs a while loop here
    {
        return false;
    }

    // burn everything in <range> (including player)
    game.appendMessagePart(WHITE_BLACK_PAIR, std::format("The fireball explodes, burning everything within {} tiles!", Fireball::maxRange));
    game.finalizeMessage();

    // Create a more dynamic, realistic fire explosion
    create_explosion(tilePicked);

    // if the player is in range of the fireball animate the player
    if (game.player->get_tile_distance(tilePicked) <= Fireball::maxRange)
    {
        animation(game.player->position, maxRange);
        // damage the player
        game.player->destructible->take_damage(*game.player, damage);
    }

    // First pass to show affected creatures and display messages
    for (const auto& c : game.creatures)
    {
        if (c)
        {
            if (!c->destructible->is_dead() && c->get_tile_distance(tilePicked) <= Fireball::maxRange)
            {
                game.appendMessagePart(WHITE_BLACK_PAIR, std::format("The {} gets engulfed in flames!", c->actorData.name));
                game.appendMessagePart(WHITE_BLACK_PAIR, std::format(" ({} damage)", damage));
                game.finalizeMessage();
                animation(c->position, maxRange);
            }
        }
    }

    // Second pass to actually apply damage (separated to prevent issues if creatures die)
    for (const auto& c : game.creatures)
    {
        if (c)
        {
            if (!c->destructible->is_dead() && c->get_tile_distance(tilePicked) <= Fireball::maxRange)
            {
                c->destructible->take_damage(*c, damage);
            }
        }
    }

    return Pickable::use(owner, wearer);
}

void Fireball::create_explosion(Vector2D center)
{
    // Get the radius of the explosion
    int radius = Fireball::maxRange;
    int fullDiameter = radius * 2 + 1;

    // Calculate the area for the explosion window
    Vector2D topLeft{ center.y - radius, center.x - radius };

    // Create an explosion window
    WINDOW* explosionWindow = newwin(
        fullDiameter, // height
        fullDiameter, // width
        topLeft.y,    // y position
        topLeft.x     // x position
    );

    // Enable non-blocking mode for animation
    nodelay(explosionWindow, true);

    // Fire colors - use a gradient of colors for more realistic fire
    const int FIRE_COLORS[] = {
        RED_YELLOW_PAIR,    // Base fire color
        RED_YELLOW_PAIR,      // Another fire-like color
        WHITE_RED_PAIR // Red for intense heat
    };
    const int COLOR_COUNT = 3;

    // Fire characters for more varied visual effect
    const char FIRE_CHARS[] = { '*', '#', '&', '@', '%', '+', '=', '^' };
    const int CHAR_COUNT = 8;

    // Fire animation phases
    const int EXPANSION_FRAMES = 10;  // Frames for expansion
    const int PEAK_FRAMES = 20;       // Frames at full size
    const int FADE_FRAMES = 15;       // Frames for fade out

    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> colorDist(0, COLOR_COUNT - 1);
    std::uniform_int_distribution<> charDist(0, CHAR_COUNT - 1);

    // Animate the explosion
    // Phase 1: Expansion
    for (int frame = 0; frame < EXPANSION_FRAMES; frame++) {
        wclear(explosionWindow);
        int currentRadius = (radius * frame) / EXPANSION_FRAMES;

        // Draw expanding circular fire
        for (int y = 0; y < fullDiameter; y++) {
            for (int x = 0; x < fullDiameter; x++) {
                // Distance from center of the explosion
                double distance = std::sqrt(std::pow(y - radius, 2) + std::pow(x - radius, 2));

                if (distance <= currentRadius) {
                    int colorIndex = colorDist(gen);
                    wattron(explosionWindow, COLOR_PAIR(FIRE_COLORS[colorIndex]));
                    mvwaddch(explosionWindow, y, x, FIRE_CHARS[charDist(gen)]);
                    wattroff(explosionWindow, COLOR_PAIR(FIRE_COLORS[colorIndex]));
                }
            }
        }
        wrefresh(explosionWindow);
        napms(30);  // Control speed of expansion
    }

    // Phase 2: Full explosion
    for (int frame = 0; frame < PEAK_FRAMES; frame++) {
        // Redraw with flickering
        for (int y = 0; y < fullDiameter; y++) {
            for (int x = 0; x < fullDiameter; x++) {
                double distance = std::sqrt(std::pow(y - radius, 2) + std::pow(x - radius, 2));

                if (distance <= radius) {
                    // More intense fire at the center, less at the edges
                    double intensity = 1.0 - (distance / radius);
                    int colorIndex;

                    // Higher chance of intense colors near center
                    if (std::bernoulli_distribution(intensity)(gen)) {
                        colorIndex = colorDist(gen) % 2;  // More intense colors
                    }
                    else {
                        colorIndex = 2;  // Less intense color
                    }

                    wattron(explosionWindow, COLOR_PAIR(FIRE_COLORS[colorIndex]));
                    mvwaddch(explosionWindow, y, x, FIRE_CHARS[charDist(gen)]);
                    wattroff(explosionWindow, COLOR_PAIR(FIRE_COLORS[colorIndex]));
                }
            }
        }
        wrefresh(explosionWindow);
        napms(50);  // Longer display at peak
    }

    // Phase 3: Fade out
    for (int frame = 0; frame < FADE_FRAMES; frame++) {
        wclear(explosionWindow);
        int currentRadius = radius - (radius * frame) / FADE_FRAMES;

        // Draw shrinking fire
        for (int y = 0; y < fullDiameter; y++) {
            for (int x = 0; x < fullDiameter; x++) {
                double distance = std::sqrt(std::pow(y - radius, 2) + std::pow(x - radius, 2));

                if (distance <= currentRadius) {
                    // As fire fades, use less intense colors
                    int colorIndex = 2; // Least intense color predominates

                    if (std::bernoulli_distribution(0.3)(gen)) {
                        colorIndex = colorDist(gen);
                    }

                    wattron(explosionWindow, COLOR_PAIR(FIRE_COLORS[colorIndex]));
                    mvwaddch(explosionWindow, y, x, FIRE_CHARS[charDist(gen)]);
                    wattroff(explosionWindow, COLOR_PAIR(FIRE_COLORS[colorIndex]));
                }
            }
        }
        wrefresh(explosionWindow);
        napms(40);  // Control speed of fade
    }

    // Clean up
    nodelay(explosionWindow, false);
    delwin(explosionWindow);

    // CRITICAL FIX: Clear screen completely then restore game display
    clear();
    refresh();
    game.restore_game_display();
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