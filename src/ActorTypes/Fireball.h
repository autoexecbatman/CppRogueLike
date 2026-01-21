#pragma once

#include <curses.h>

#include "../Persistent/Persistent.h"
#include "LightningBolt.h"

class Item;
class Creature;
struct Vector2D;
struct GameContext;

// Constants moved to class scope
static constexpr int FIRE_COLOR_COUNT = 3;
static constexpr int FIRE_CHAR_COUNT = 8;

struct ExplosionData
{
    WINDOW* window{};
    int radius{};
    int fullDiameter{};
};


//==FIREBALL==
//==
class Fireball : public LightningBolt
{
public:
    Fireball(int range, int damage);

    bool use(Item& owner, Creature& wearer, GameContext& ctx) override;

    // Creates a dynamic fire explosion effect
    void create_explosion(Vector2D center, GameContext& ctx);

    ExplosionData initialize_explosion_data(Vector2D center);
    void animate_expansion_phase(const ExplosionData & explosion, GameContext& ctx);
    void animate_peak_phase(const ExplosionData & explosion, GameContext& ctx);
    void animate_fade_phase(const ExplosionData & explosion, GameContext& ctx);
    void draw_explosion_frame(const ExplosionData & explosion, int currentRadius, GameContext& ctx);
    void draw_flickering_explosion(const ExplosionData & explosion, GameContext& ctx);
    void draw_fading_explosion(const ExplosionData & explosion, int currentRadius, GameContext& ctx);
    bool is_within_explosion_radius(int y, int x, int centerRadius, int currentRadius);
    double calculate_distance_from_center(int y, int x, int centerRadius);
    int get_intensity_based_color(double intensity, GameContext& ctx);
    int get_fade_color(GameContext& ctx);
    void draw_fire_character(WINDOW * window, int y, int x, int colorIndex, GameContext& ctx);
    void cleanup_explosion_window(WINDOW * window, GameContext& ctx);

    // Individual creature animation for being hit by fire
    void animation(Vector2D position, int maxRange);

    void load(const json& j);
    void save(json& j);
};
//====