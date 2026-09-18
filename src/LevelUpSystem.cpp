// LevelUpSystem.cpp - Handles combat improvements on level up according to AD&D 2e rules
#include <algorithm>
#include <format>
#include <string>

#include "Creature.h"
#include "Colors.h"
#include "GameContext.h"
#include "CombatProgressionTables.h"
#include "DataManager.h"
#include "MessageSystem.h"
#include "TurningTable.h"
#include "GameBalance.h"
#include "LevelUpSystem.h"
#include "SavingThrow.h"

// ============================================================================
// Private implementation — not visible outside this translation unit.
// ============================================================================

namespace
{

void apply_thac0_improvement(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    const int newTHAC0 = LevelUpSystem::thac0_for_level(owner.get_creature_class(), newLevel);

    if (newTHAC0 < owner.get_thaco())
    {
        int oldTHAC0 = owner.get_thaco();
        owner.set_thaco(newTHAC0);

        if (owner.get_creature_class() != CreatureClass::MONSTER)
        {
            ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, "THAC0 improved");
            ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR,
                std::format(" from {} to {}!", oldTHAC0, newTHAC0));
            ctx->messageSystem->finalize_message();
        }

        ctx->messageSystem->log(std::format("THAC0 improved: {} -> {}", oldTHAC0, newTHAC0));
    }
}

int apply_hit_point_gain(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return 0;
    }

    auto roll_hit_die = [&]() -> int
    {
        switch (owner.get_hit_die())
        {
        case 4:
        {
            return ctx->dice->d4();
        }

        case 6:
        {
            return ctx->dice->d6();
        }

        case 10:
        {
            return ctx->dice->d10();
        }

        default:
        {
            return ctx->dice->d8();
        }
        }
    };

    // AD&D 2e: a character rolls hit dice only up to a class-dependent level.
    // Past it the book grants a flat number of hit points per level, and the
    // Constitution bonus stops applying entirely.
    const LevelUpSystem::HitPointProgression progression = LevelUpSystem::hit_point_progression(owner.get_creature_class());
    const bool stillRollsDice = newLevel <= progression.lastRolledLevel;

    int hitDiceRoll = stillRollsDice ? roll_hit_die() : progression.flatGain;
    std::string diceType = stillRollsDice
        ? std::format("d{}", owner.get_hit_die())
        : std::string{ "fixed" };

    int conBonus = 0;
    if (stillRollsDice)
    {
        conBonus = ctx->dataManager->constitution_for(owner.get_constitution()).HPAdj;
    }

    int totalHPGain = std::max(1, hitDiceRoll + conBonus);

    owner.set_hp_base(owner.get_hp_base() + hitDiceRoll);
    owner.set_max_hp(owner.get_max_hp() + totalHPGain);
    owner.set_hp(owner.get_hp() + totalHPGain);

    if (owner.get_creature_class() != CreatureClass::MONSTER)
    {
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, "Hit Points increased");
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " by ");
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, std::to_string(totalHPGain));
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " (");
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, diceType);
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, ": ");
        ctx->messageSystem->append_message_part(YELLOW_BLACK_PAIR, std::to_string(hitDiceRoll));
        if (conBonus != 0)
        {
            ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " + ");
            ctx->messageSystem->append_message_part(YELLOW_BLACK_PAIR, std::to_string(conBonus));
            ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " CON");
        }
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, ")");
        ctx->messageSystem->finalize_message();
    }

    ctx->messageSystem->log(std::format("HP increased by {} ({} rolled + {} CON bonus). Max HP now: {}",
        totalHPGain,
        hitDiceRoll,
        conBonus,
        owner.get_max_hp()));

    return totalHPGain;
}

void apply_fighter_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    if (newLevel >= 13)
    {
        if (owner.get_attacks_per_round() < 2.0f)
        {
            owner.set_attacks_per_round(2.0f);
            ctx->messageSystem->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
            ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, "Extra Attack!");
            ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " You can now attack 2 times per round.");
            ctx->messageSystem->finalize_message();
            ctx->messageSystem->log("Fighter gained extra attack (2 attacks per round)");
        }
    }
    else if (newLevel >= 7)
    {
        if (owner.get_attacks_per_round() < 1.5f)
        {
            owner.set_attacks_per_round(1.5f);
            ctx->messageSystem->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
            ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, "Extra Attack!");
            ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " You can now attack 3/2 times per round.");
            ctx->messageSystem->finalize_message();
            ctx->messageSystem->log("Fighter gained extra attack (3/2 attacks per round)");
        }
    }

    if (newLevel % 3 == 0)
    {
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, "Your martial prowess improves!");
        ctx->messageSystem->finalize_message();
    }
}

void apply_rogue_improvements(int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    int backstabMultiplier = LevelUpSystem::calculate_backstab_multiplier(newLevel);
    if (backstabMultiplier > LevelUpSystem::calculate_backstab_multiplier(newLevel - 1))
    {
        ctx->messageSystem->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, "Backstab improved!");
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " Damage multiplier: x");
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, std::to_string(backstabMultiplier));
        ctx->messageSystem->finalize_message();
        ctx->messageSystem->log(std::format("Rogue backstab multiplier increased to x{}", backstabMultiplier));
    }

    if (newLevel % 2 == 0)
    {
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, "Your thieving skills improve!");
        ctx->messageSystem->finalize_message();
    }
}

void apply_cleric_improvements(int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    // Announce only when a tougher class of undead comes into reach. The target
    // numbers improve at almost every level, so announcing those would announce
    // almost every level.
    const int reachNow = highest_turnable_hit_dice(newLevel);
    if (reachNow > highest_turnable_hit_dice(newLevel - 1))
    {
        ctx->messageSystem->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, "Turn Undead improved!");
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " You can now turn undead of up to ");
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, std::to_string(reachNow));
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " hit dice.");
        ctx->messageSystem->finalize_message();
        ctx->messageSystem->log(std::format("Cleric turning reach rose to {} HD at level {}", reachNow, newLevel));
    }

    if (newLevel >= 2)
    {
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, "Your divine power grows stronger!");
        ctx->messageSystem->finalize_message();
    }
}

void apply_wizard_improvements(int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    if (newLevel % 2 == 1 && newLevel > 1)
    {
        int spellLevel = (newLevel + 1) / 2;
        if (spellLevel <= 9)
        {
            ctx->messageSystem->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
            ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, "New spell level!");
            ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " You can now cast level ");
            ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, std::to_string(spellLevel));
            ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " spells.");
            ctx->messageSystem->finalize_message();
            ctx->messageSystem->log(std::format("Wizard can now cast level {} spells", spellLevel));
        }
    }

    ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, "Your arcane knowledge deepens!");
    ctx->messageSystem->finalize_message();
}

void apply_class_specific_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    switch (owner.get_creature_class())
    {
    case CreatureClass::FIGHTER:
    {
        apply_fighter_improvements(owner, newLevel, ctx);
        break;
    }

    case CreatureClass::ROGUE:
    {
        apply_rogue_improvements(newLevel, ctx);
        break;
    }

    case CreatureClass::CLERIC:
    {
        apply_cleric_improvements(newLevel, ctx);
        break;
    }

    case CreatureClass::WIZARD:
    {
        apply_wizard_improvements(newLevel, ctx);
        break;
    }

    case CreatureClass::MONSTER:
    {
        break; // No class-specific improvements for monsters currently
    }
    }
}

void apply_ability_score_improvement(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    ctx->messageSystem->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
    ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, "Ability Score Improvement!");
    ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, " You may increase one ability score by 1 point.");
    ctx->messageSystem->finalize_message();

    switch (owner.get_creature_class())
    {
    case CreatureClass::FIGHTER:
    {
        owner.set_strength(std::min(18, owner.get_strength() + 1));
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR,
            std::format("Strength increased to {}!", owner.get_strength()));
        ctx->messageSystem->finalize_message();
        break;
    }

    case CreatureClass::ROGUE:
    {
        owner.set_dexterity(std::min(18, owner.get_dexterity() + 1));
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR,
            std::format("Dexterity increased to {}!", owner.get_dexterity()));
        ctx->messageSystem->finalize_message();
        break;
    }

    case CreatureClass::CLERIC:
    {
        owner.set_wisdom(std::min(18, owner.get_wisdom() + 1));
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR,
            std::format("Wisdom increased to {}!", owner.get_wisdom()));
        ctx->messageSystem->finalize_message();
        break;
    }

    case CreatureClass::WIZARD:
    {
        owner.set_intelligence(std::min(18, owner.get_intelligence() + 1));
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR,
            std::format("Intelligence increased to {}!", owner.get_intelligence()));
        ctx->messageSystem->finalize_message();
        break;
    }

    case CreatureClass::MONSTER:
    {
        break;
    }
    }

    ctx->messageSystem->log(std::format("Ability score improved at level {}", newLevel));
}

void apply_saving_throw_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    // Table 60 is banded, and a level improves the save when it lands on a row
    // the level below did not. Asking the table means this cannot drift from it.
    const bool improved = SavingThrows::improves_at(owner.get_creature_class(), newLevel);

    if (improved)
    {
        if (owner.get_creature_class() != CreatureClass::MONSTER)
        {
            ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, "Saving throws improved!");
            ctx->messageSystem->finalize_message();
        }
        ctx->messageSystem->log(std::format("Saving throws improved at level {}", newLevel));
    }
}

} // anonymous namespace

// ============================================================================
// Public interface
// ============================================================================

namespace LevelUpSystem
{

void apply_level_up_benefits(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx)
    {
        return;
    }

    int oldTHAC0 = owner.get_thaco();

    apply_thac0_improvement(owner, newLevel, ctx);
    int hpGained = apply_hit_point_gain(owner, newLevel, ctx);
    apply_class_specific_improvements(owner, newLevel, ctx);

    if (newLevel % 4 == 0 && owner.get_creature_class() != CreatureClass::MONSTER)
    {
        apply_ability_score_improvement(owner, newLevel, ctx);
    }

    apply_saving_throw_improvements(owner, newLevel, ctx);

    bool thac0_improved = (oldTHAC0 != owner.get_thaco());

    if (owner.get_creature_class() != CreatureClass::MONSTER)
    {
        ctx->messageSystem->append_message_part(YELLOW_BLACK_PAIR, "LEVEL UP! ");
        ctx->messageSystem->append_message_part(WHITE_BLACK_PAIR, std::format("You are now level {}. ", newLevel));
        ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR, std::format("+{} HP, ", hpGained));
        if (thac0_improved)
        {
            ctx->messageSystem->append_message_part(GREEN_BLACK_PAIR,
                std::format("THAC0 {}->{}", oldTHAC0, owner.get_thaco()));
        }
        ctx->messageSystem->finalize_message();
        ctx->messageSystem->log(std::format("Level {} reached! Combat abilities improved.", newLevel));
    }
    else
    {
        ctx->messageSystem->log(std::format("{} reaches level {}.", owner.actorData.name, newLevel));
    }
}

int calculate_backstab_multiplier(int level)
{
    if (level >= 13)
    {
        return 5;
    }
    if (level >= 9)
    {
        return 4;
    }
    if (level >= 5)
    {
        return 3;
    }
    if (level >= 1)
    {
        return 2;
    }
    return 1;
}

} // namespace LevelUpSystem

// Which level a class stops rolling hit dice at, and what it gains per level
// after that. AD&D 2e Player's Handbook Tables 14, 20, 23 and 25.
//
// Monsters do not advance by these tables; they take the warrior progression so
// the function has a defined answer for every class.
//
// Example:
//   hit_point_progression(CreatureClass::WIZARD).lastRolledLevel; // -> 10
//   hit_point_progression(CreatureClass::WIZARD).flatGain;        // -> 1
LevelUpSystem::HitPointProgression LevelUpSystem::hit_point_progression(CreatureClass creatureClass)
{
    using namespace GameBalance::Leveling::HitPoints;

    switch (creatureClass)
    {
    case CreatureClass::ROGUE:
    {
        return LevelUpSystem::HitPointProgression{ ROGUE_LAST_ROLLED_LEVEL, ROGUE_FLAT_GAIN };
    }

    case CreatureClass::CLERIC:
    {
        return LevelUpSystem::HitPointProgression{ CLERIC_LAST_ROLLED_LEVEL, CLERIC_FLAT_GAIN };
    }

    case CreatureClass::WIZARD:
    {
        return LevelUpSystem::HitPointProgression{ WIZARD_LAST_ROLLED_LEVEL, WIZARD_FLAT_GAIN };
    }

    case CreatureClass::FIGHTER:
    case CreatureClass::MONSTER:
    {
        return LevelUpSystem::HitPointProgression{ FIGHTER_LAST_ROLLED_LEVEL, FIGHTER_FLAT_GAIN };
    }
    }

    return LevelUpSystem::HitPointProgression{ FIGHTER_LAST_ROLLED_LEVEL, FIGHTER_FLAT_GAIN };
}

// The THAC0 a class attacks at on a given level. AD&D 2e Player's Handbook
// attack tables, one per class, with monsters on the warrior's.
//
// Example:
//   thac0_for_level(CreatureClass::WIZARD, 3);   // -> 20
//   thac0_for_level(CreatureClass::WIZARD, 4);   // -> 19
int LevelUpSystem::thac0_for_level(CreatureClass creatureClass, int level)
{
    static constexpr CombatProgressionTables combatTables;

    switch (creatureClass)
    {
    case CreatureClass::ROGUE:
    {
        return combatTables.get_rogue(level);
    }

    case CreatureClass::CLERIC:
    {
        return combatTables.get_cleric(level);
    }

    case CreatureClass::WIZARD:
    {
        return combatTables.get_wizard(level);
    }

    case CreatureClass::FIGHTER:
    case CreatureClass::MONSTER:
    {
        return combatTables.get_fighter(level);
    }
    }

    return combatTables.get_fighter(level);
}

// Whether reaching this level moves the class down its attack table.
//
// Example:
//   thac0_improves_at(CreatureClass::FIGHTER, 3);  // -> true
//   thac0_improves_at(CreatureClass::WIZARD, 3);   // -> false, 20 at both
bool LevelUpSystem::thac0_improves_at(CreatureClass creatureClass, int level)
{
    // Level 1 has no level below it to improve from; the table answers 20 there,
    // which is what a class starts at, so the comparison already says no.
    return thac0_for_level(creatureClass, level) < thac0_for_level(creatureClass, level - 1);
}
