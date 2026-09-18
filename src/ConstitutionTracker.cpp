#include <algorithm>
#include "ConstitutionTracker.h"

#include "Creature.h"
#include "ConstitutionAttributes.h"
#include "GameContext.h"
#include "DataManager.h"

// Player's Handbook Table 3 grants more than +2 to warriors alone.
static constexpr int NON_WARRIOR_BONUS_CAP = 2;

[[nodiscard]] int ConstitutionTracker::calculate_constitution_hp_bonus_for_value(
    int constitution,
    CreatureClass creatureClass,
    GameContext& ctx) const
{
    const auto& constitutionAttributes = ctx.dataManager->get_constitution_attributes();

    if (constitution < 1 || constitution > static_cast<int>(constitutionAttributes.size()))
    {
        return 0;
    }

    const int tableAdjustment = constitutionAttributes[constitution - 1].HPAdj;

    // The table is the warrior column. Only a bonus is capped, so a penalty
    // passes through untouched for every class.
    if (is_warrior(creatureClass))
    {
        return tableAdjustment;
    }
    return std::min(tableAdjustment, NON_WARRIOR_BONUS_CAP);
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
    const std::optional<int> lastCon = get_last_constitution();

    ConstitutionChangeResult result{};

    if (lastCon.has_value() && *lastCon == currentConstitution)
    {
        return result;
    }

    // With no earlier score the whole bonus is applied and nothing has changed.
    result.firstApplication = !lastCon.has_value();
    const int oldBonus = lastCon.has_value() ? calculate_constitution_hp_bonus_for_value(*lastCon, owner.get_creature_class(), ctx) : 0;
    const int newBonus = calculate_constitution_hp_bonus_for_value(currentConstitution, owner.get_creature_class(), ctx);
    const int level = calculate_level_multiplier(owner);
    const int hpDifference = (newBonus - oldBonus) * level;

    result.oldBonus = oldBonus;
    result.newBonus = newBonus;
    result.hpDifference = hpDifference;

    set_last_constitution(currentConstitution);

    return result;
}
