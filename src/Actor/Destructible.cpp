// file: Destructible.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <string>

#include "../Actor/Creature.h"
#include "../Attributes/ConstitutionAttributes.h"
#include "../Attributes/DexterityAttributes.h"
#include "../Items/ItemIdentification.h"
#include "../Colors/Colors.h"
#include "../Combat/DamageInfo.h"
#include "../Combat/DamageResolver.h"
#include "../Core/GameContext.h"
#include "../Persistent/Persistent.h"
#include "../Systems/BuffSystem.h"
#include "../Systems/DataManager.h"
#include "../Systems/FloatingTextSystem.h"
#include "../Systems/MessageSystem.h"
#include "Destructible.h"
#include "EquipmentSlot.h"
#include "Pickable.h"

Destructible::Destructible(int hpMax)
    : constitutionTracker(std::make_unique<ConstitutionTracker>()),
      healthPool(std::make_unique<HealthPool>(hpMax))
{
}

void Destructible::update_constitution_bonus(Creature& owner, GameContext& ctx)
{
    const int oldCon = get_last_constitution();
    const auto result = constitutionTracker->apply_constitution_changes(owner, ctx);

    if (result.hpDifference == 0)
    {
        return;
    }

    set_max_hp(get_max_hp() + result.hpDifference);
    set_hp(get_hp() + result.hpDifference);

    log_constitution_change(owner, ctx, oldCon, owner.get_constitution(), result.hpDifference);

    if (get_hp() <= 0)
    {
        set_hp(0);
        handle_stat_drain_death(owner, ctx);
    }
}

void Destructible::handle_stat_drain_death(Creature& owner, GameContext& ctx)
{
    if (&owner == ctx.player)
    {
        ctx.messageSystem->message(RED_BLACK_PAIR, "Your life force has been drained beyond recovery. You die!", true);
    }
    else
    {
        ctx.messageSystem->log(std::format("{} dies from stat drain.", owner.get_name()));
    }

    owner.die(ctx);
}

void Destructible::log_constitution_change(const Creature& owner, GameContext& ctx, int oldCon, int newCon, int hpChange) const
{
    if (&owner != ctx.player)
    {
        return;
    }

    if (hpChange > 0)
    {
        ctx.messageSystem->message(GREEN_BLACK_PAIR,
            std::format("Constitution increased from {} to {}! You gain {} hit points.", oldCon, newCon, hpChange),
            true);
    }
    else
    {
        ctx.messageSystem->message(RED_BLACK_PAIR,
            std::format("Constitution decreased from {} to {}! You lose {} hit points.", oldCon, newCon, -hpChange),
            true);
    }
}


int Destructible::take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType)
{
    const int actual = healthPool->take_damage(owner, damage, ctx, damageType);
    if (is_dead())
    {
        owner.die(ctx);
    }
    return actual;
}

void Destructible::load(const json& j)
{
    healthPool->set_max_hp(j.at("hpMax").get<int>());
    healthPool->set_hp(j.at("hp").get<int>());
    healthPool->set_hp_base(j.at("hpBase").get<int>());
    healthPool->set_temp_hp(j.at("tempHp").get<int>());
    constitutionTracker->set_last_constitution(j.at("lastConstitution").get<int>());
}

void Destructible::save(json& j)
{
    j["hpMax"] = healthPool->get_max_hp();
    j["hp"] = healthPool->get_hp();
    j["hpBase"] = healthPool->get_hp_base();
    j["tempHp"] = healthPool->get_temp_hp();
    j["lastConstitution"] = get_last_constitution();
}

[[nodiscard]] std::unique_ptr<Destructible> Destructible::create(const json& j)
{
    auto destructible = std::make_unique<Destructible>(0);
    destructible->load(j);
    return destructible;
}

// end of file: Destructible.cpp
