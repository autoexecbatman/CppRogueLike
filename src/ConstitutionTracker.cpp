#include "ConstitutionTracker.h"

#include "Creature.h"
#include "GameContext.h"
#include "DataManager.h"
#include "LevelUpSystem.h"

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
    const int oldBonus = lastCon.has_value() ? ctx.dataManager->constitution_hit_point_adjustment(*lastCon, owner.get_creature_class()) : 0;
    const int newBonus = ctx.dataManager->constitution_hit_point_adjustment(currentConstitution, owner.get_creature_class());
    const int levels = LevelUpSystem::levels_taking_constitution_adjustment(owner.get_creature_class(), owner.get_creature_level());
    const int hpDifference = (newBonus - oldBonus) * levels;

    result.oldBonus = oldBonus;
    result.newBonus = newBonus;
    result.hpDifference = hpDifference;

    set_last_constitution(currentConstitution);

    return result;
}
