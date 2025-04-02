#pragma once

#include <libtcod.h>

#include "../Actor/Actor.h"
#include "LightningBolt.h"

//==FIREBALL==
//==
class Fireball : public LightningBolt
{
public:
    Fireball(int range, int damage);

    bool use(Item& owner, Creature& wearer) override;

    // Creates a dynamic fire explosion effect
    void create_explosion(Vector2D center);

    // Individual creature animation for being hit by fire
    void animation(Vector2D position, int maxRange);

    void load(const json& j);
    void save(json& j);
};
//====