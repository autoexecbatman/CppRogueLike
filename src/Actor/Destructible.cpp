// file: Destructible.cpp
#include "../Actor/Creature.h"
#include "../Combat/DamageInfo.h"
#include "../Core/GameContext.h"
#include "Destructible.h"


int Destructible::take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType)
{
    const int actual = owner.take_damage(damage, ctx, damageType);
    if (owner.is_dead())
    {
        owner.die(ctx);
    }
    return actual;
}

void Destructible::load(const json& j)
{
    // Destructible is now just a thin coordinator; constitution tracking moved to Creature
}

void Destructible::save(json& j)
{
    // Destructible is now just a thin coordinator; constitution tracking moved to Creature
}

[[nodiscard]] std::unique_ptr<Destructible> Destructible::create(const json& j)
{
    auto destructible = std::make_unique<Destructible>();
    destructible->load(j);
    return destructible;
}
