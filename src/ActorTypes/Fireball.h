#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "LightningBolt.h"

// Constants moved to class scope
static constexpr int FIRE_COLOR_COUNT = 3;
static constexpr int FIRE_CHAR_COUNT = 8;

struct ExplosionData
{
    WINDOW* window;
    int radius;
    int fullDiameter;
};


//==FIREBALL==
//==
class Fireball : public LightningBolt
{
public:
    Fireball(int range, int damage);

    bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

    // Creates a dynamic fire explosion effect
    void create_explosion(Vector2D center);

    ExplosionData initialize_explosion_data(Vector2D center);
    void animate_expansion_phase(const ExplosionData & explosion);
    void animate_peak_phase(const ExplosionData & explosion);
    void animate_fade_phase(const ExplosionData & explosion);
    void draw_explosion_frame(const ExplosionData & explosion, int currentRadius);
    void draw_flickering_explosion(const ExplosionData & explosion);
    void draw_fading_explosion(const ExplosionData & explosion, int currentRadius);
    bool is_within_explosion_radius(int y, int x, int centerRadius, int currentRadius);
    double calculate_distance_from_center(int y, int x, int centerRadius);
    int get_intensity_based_color(double intensity);
    int get_fade_color();
    void draw_fire_character(WINDOW * window, int y, int x, int colorIndex);
    void cleanup_explosion_window(WINDOW * window);

    // Individual creature animation for being hit by fire
    void animation(Vector2D position, int maxRange);

    void load(const json& j);
    void save(json& j);
};
//====