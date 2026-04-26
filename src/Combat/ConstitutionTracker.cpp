#include "ConstitutionTracker.h"

#include "../Actor/Creature.h"
#include "../Attributes/ConstitutionAttributes.h"
#include "../Core/GameContext.h"
#include "../Systems/DataManager.h"

[[nodiscard]] int ConstitutionTracker::calculate_constitution_hp_bonus_for_value(
    int constitution,
    GameContext& ctx) const
{
    const auto& constitutionAttributes = ctx.dataManager->get_constitution_attributes();

    if (constitution < 1 || constitution > static_cast<int>(constitutionAttributes.size()))
    {
        return 0;
    }

    return constitutionAttributes[constitution - 1].HPAdj;
}

[[nodiscard]] int ConstitutionTracker::calculate_level_multiplier(const Creature& owner) const
{
    return owner.get_constitution_hp_multiplier();
}

[[nodiscard]] ConstitutionTracker::ConstitutionChangeResult ConstitutionTracker::apply_constitution_changes(
    Creature& owner,
    GameContext& ctx)
{
    const int currentConstitution = owner.get_constitution();
    const int lastCon = get_last_constitution();

    ConstitutionChangeResult result{};

    if (currentConstitution == lastCon)
    {
        return result;
    }

    const int oldBonus = calculate_constitution_hp_bonus_for_value(lastCon, ctx);
    const int newBonus = calculate_constitution_hp_bonus_for_value(currentConstitution, ctx);
    const int level = calculate_level_multiplier(owner);
    const int hpDifference = (newBonus - oldBonus) * level;

    result.oldBonus = oldBonus;
    result.newBonus = newBonus;
    result.hpDifference = hpDifference;

    set_last_constitution(currentConstitution);

    return result;
}
