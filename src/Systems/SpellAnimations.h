#pragma once

struct Vector2D;
struct GameContext;

namespace SpellAnimations
{
    void animate_lightning(Vector2D from, Vector2D to, GameContext& ctx);
    void animate_explosion(Vector2D center, int radius, GameContext& ctx);
    void animate_creature_hit(Vector2D position);
}
