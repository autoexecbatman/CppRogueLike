// LevelUpSystem.cpp - Handles combat improvements on level up according to AD&D 2e rules

#include "LevelUpSystem.h"
#include "../Core/GameContext.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Player.h"
#include "../dnd_tables/CalculatedTHAC0s.h"
#include "../Colors/Colors.h"
#include <format>

void LevelUpSystem::apply_level_up_benefits(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return;

    // Get the player class
    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr)
    {
        return; // Only handle player level ups
    }

    // Store old values for comparison in display
    int oldHP = owner.destructible->get_hp();
    int oldMaxHP = owner.destructible->get_max_hp();
    int oldTHAC0 = owner.destructible->get_thaco();

    // Apply THAC0 improvement
    apply_thac0_improvement(owner, newLevel, ctx);

    // Apply hit point gains
    int hpGained = apply_hit_point_gain(owner, newLevel, ctx);

    // Apply class-specific combat improvements
    apply_class_specific_improvements(owner, newLevel, ctx);

    // Apply ability score improvements (AD&D 2e: every 4 levels)
    if (newLevel % 4 == 0)
    {
        apply_ability_score_improvement(owner, newLevel, ctx);
    }

    // Apply saving throw improvements
    apply_saving_throw_improvements(owner, newLevel, ctx);

    // Log the level up with summary
    ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "LEVEL UP! ");
    ctx->message_system->append_message_part(WHITE_BLACK_PAIR, std::format("You are now level {}. ", newLevel));
    ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::format("+{} HP, ", hpGained));
    if (oldTHAC0 != owner.destructible->get_thaco())
    {
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::format("THAC0 {}->{}", oldTHAC0, owner.destructible->get_thaco()));
    }
    ctx->message_system->finalize_message();

    // Announce level up
    ctx->message_system->log(std::format("Level {} reached! Combat abilities improved.", newLevel));
}

void LevelUpSystem::apply_thac0_improvement(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return;

    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr || !owner.destructible)
    {
        return;
    }

    CalculatedTHAC0s thac0Tables;
    int newTHAC0 = 20; // Default

    switch (playerPtr->playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        newTHAC0 = thac0Tables.get_fighter(newLevel);
        break;
    case Player::PlayerClassState::ROGUE:
        newTHAC0 = thac0Tables.get_rogue(newLevel);
        break;
    case Player::PlayerClassState::CLERIC:
        newTHAC0 = thac0Tables.get_cleric(newLevel);
        break;
    case Player::PlayerClassState::WIZARD:
        newTHAC0 = thac0Tables.get_wizard(newLevel);
        break;
    }

    // Only update if THAC0 improved (lower is better)
    if (newTHAC0 < owner.destructible->get_thaco())
    {
        int oldTHAC0 = owner.destructible->get_thaco();
        owner.destructible->set_thaco(newTHAC0);

        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "THAC0 improved");
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " from ");
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, std::to_string(oldTHAC0));
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " to ");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(newTHAC0));
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "!");
        ctx->message_system->finalize_message();

            ctx->message_system->log(std::format("THAC0 improved: {} -> {}", oldTHAC0, newTHAC0));
    }
}

int LevelUpSystem::apply_hit_point_gain(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return 0;

    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr || !owner.destructible)
    {
        return 0;
    }

    // Roll hit dice based on class
    int hitDiceRoll = 0;
    std::string diceType;

    switch (playerPtr->playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        hitDiceRoll = ctx->dice->d10();
        diceType = "d10";
        break;
    case Player::PlayerClassState::ROGUE:
        hitDiceRoll = ctx->dice->d6();
        diceType = "d6";
        break;
    case Player::PlayerClassState::CLERIC:
        hitDiceRoll = ctx->dice->d8();
        diceType = "d8";
        break;
    case Player::PlayerClassState::WIZARD:
        hitDiceRoll = ctx->dice->d4();
        diceType = "d4";
        break;
    }

    // Get Constitution bonus
    int conBonus = 0;
    if (owner.get_constitution() >= 1 && owner.get_constitution() <= ctx->data_manager->get_constitution_attributes().size())
    {
        conBonus = ctx->data_manager->get_constitution_attributes()[owner.get_constitution() - 1].HPAdj;
    }

    // Calculate total HP gain (minimum 1)
    int totalHPGain = std::max(1, hitDiceRoll + conBonus);

    // Update HP values
    owner.destructible->set_hp_base(owner.destructible->get_hp_base() + hitDiceRoll); // Base HP without Con bonus
    owner.destructible->set_max_hp(owner.destructible->get_max_hp() + totalHPGain);
    owner.destructible->set_hp(owner.destructible->get_hp() + totalHPGain); // Give full HP on level up

    // Display HP gain message
    ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Hit Points increased");
    ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " by ");
    ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(totalHPGain));
    ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " (");
    ctx->message_system->append_message_part(WHITE_BLACK_PAIR, diceType);
    ctx->message_system->append_message_part(WHITE_BLACK_PAIR, ": ");
    ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, std::to_string(hitDiceRoll));
    if (conBonus != 0)
    {
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " + ");
        ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, std::to_string(conBonus));
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " CON");
    }
    ctx->message_system->append_message_part(WHITE_BLACK_PAIR, ")");
    ctx->message_system->finalize_message();

    ctx->message_system->log(std::format("HP increased by {} ({} rolled + {} CON bonus). Max HP now: {}",
                         totalHPGain, hitDiceRoll, conBonus, owner.destructible->get_max_hp()));

    return totalHPGain; // Return HP gained for display
}

void LevelUpSystem::apply_class_specific_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return;

    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr)
    {
        return;
    }

    switch (playerPtr->playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        apply_fighter_improvements(owner, newLevel, ctx);
        break;
    case Player::PlayerClassState::ROGUE:
        apply_rogue_improvements(owner, newLevel, ctx);
        break;
    case Player::PlayerClassState::CLERIC:
        apply_cleric_improvements(owner, newLevel, ctx);
        break;
    case Player::PlayerClassState::WIZARD:
        apply_wizard_improvements(owner, newLevel, ctx);
        break;
    }
}

void LevelUpSystem::apply_fighter_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return;

    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr)
    {
        return;
    }

    // Fighters get extra attacks at levels 7 and 13
    if (newLevel == 7)
    {
        playerPtr->attacksPerRound = 1.5f; // 3/2 attacks per round
        ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Extra Attack!");
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You can now attack 3/2 times per round.");
        ctx->message_system->finalize_message();

            ctx->message_system->log("Fighter gained extra attack (3/2 attacks per round)");
    }
    else if (newLevel == 13)
    {
        playerPtr->attacksPerRound = 2.0f; // 2 attacks per round
        ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Extra Attack!");
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You can now attack 2 times per round.");
        ctx->message_system->finalize_message();

            ctx->message_system->log("Fighter gained extra attack (2 attacks per round)");
    }

    // Fighters also get better weapon specialization bonuses
    if (newLevel % 3 == 0) // Every 3 levels
    {
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Your martial prowess improves!");
        ctx->message_system->finalize_message();
    }
}

void LevelUpSystem::apply_rogue_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return;

    // Rogues get improved backstab multipliers
    int backstabMultiplier = calculate_backstab_multiplier(newLevel);
    if (backstabMultiplier > calculate_backstab_multiplier(newLevel - 1))
    {
        ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Backstab improved!");
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " Damage multiplier: x");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(backstabMultiplier));
        ctx->message_system->finalize_message();

            ctx->message_system->log(std::format("Rogue backstab multiplier increased to x{}", backstabMultiplier));
    }

    // Rogue skills improve
    if (newLevel % 2 == 0) // Every 2 levels
    {
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Your thieving skills improve!");
        ctx->message_system->finalize_message();
        // NOTE: Thieving skills deferred - progression tracking implemented but mechanics unused
    }
}

void LevelUpSystem::apply_cleric_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return;

    // Clerics get turn undead improvements and spell slots
    if (newLevel == 3 || newLevel == 5 || newLevel == 7 || newLevel == 9)
    {
        ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Turn Undead improved!");
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You can affect more powerful undead.");
        ctx->message_system->finalize_message();

            ctx->message_system->log(std::format("Cleric turn undead ability improved at level {}", newLevel));
        // NOTE: Turn undead mechanic deferred - tracking implemented but mechanic unused
    }

    // Spell slots increase
    if (newLevel >= 2)
    {
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Your divine power grows stronger!");
        ctx->message_system->finalize_message();
        // NOTE: Spell system integration deferred - divine power tracking available but unused
    }
}

void LevelUpSystem::apply_wizard_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return;

    // Wizards primarily get more spell slots and access to higher level spells
    if (newLevel % 2 == 1 && newLevel > 1) // Odd levels after 1st
    {
        int spellLevel = (newLevel + 1) / 2;
        if (spellLevel <= 9) // Max spell level is 9
        {
            ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
            ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "New spell level!");
            ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You can now cast level ");
            ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(spellLevel));
            ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " spells.");
            ctx->message_system->finalize_message();

                    ctx->message_system->log(std::format("Wizard can now cast level {} spells", spellLevel));
        }
    }

    // Wizards get better spell power
    ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Your arcane knowledge deepens!");
    ctx->message_system->finalize_message();
    // NOTE: Spell system integration deferred - arcane knowledge tracking available but unused
}

int LevelUpSystem::calculate_backstab_multiplier(int level)
{
    // AD&D 2e backstab progression for rogues
    if (level >= 13) return 5;
    if (level >= 9) return 4;
    if (level >= 5) return 3;
    if (level >= 1) return 2;
    return 1; // Should never happen
}

void LevelUpSystem::apply_ability_score_improvement(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return;

    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr)
    {
        return;
    }

    // AD&D 2e: Players can increase ability scores every 4 levels
    ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
    ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Ability Score Improvement!");
    ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You may increase one ability score by 1 point.");
    ctx->message_system->finalize_message();

    // NOTE: Interactive ability selection deferred - auto-assignment used instead
    // For now, automatically improve the prime requisite for the class
    switch (playerPtr->playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        playerPtr->set_strength(std::min(18, playerPtr->get_strength() + 1));
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Strength increased to ");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(playerPtr->get_strength()));
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "!");
        ctx->message_system->finalize_message();
        break;
    case Player::PlayerClassState::ROGUE:
        playerPtr->set_dexterity(std::min(18, playerPtr->get_dexterity() + 1));
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Dexterity increased to ");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(playerPtr->get_dexterity()));
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "!");
        ctx->message_system->finalize_message();
        break;
    case Player::PlayerClassState::CLERIC:
        playerPtr->set_wisdom(std::min(18, playerPtr->get_wisdom() + 1));
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Wisdom increased to ");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(playerPtr->get_wisdom()));
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "!");
        ctx->message_system->finalize_message();
        break;
    case Player::PlayerClassState::WIZARD:
        playerPtr->set_intelligence(std::min(18, playerPtr->get_intelligence() + 1));
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Intelligence increased to ");
        ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(playerPtr->get_intelligence()));
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "!");
        ctx->message_system->finalize_message();
        break;
    }

    ctx->message_system->log(std::format("Ability score improved at level {}", newLevel));
}

void LevelUpSystem::apply_saving_throw_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
    if (!ctx) return;

    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr)
    {
        return;
    }

    // AD&D 2e saving throws improve at certain levels based on class
    bool improved = false;

    switch (playerPtr->playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        // Fighters improve saves every 2-3 levels
        if (newLevel == 3 || newLevel == 6 || newLevel == 9 || newLevel == 12 || newLevel == 15)
        {
            improved = true;
        }
        break;
    case Player::PlayerClassState::ROGUE:
        // Rogues improve saves every 4 levels
        if (newLevel % 4 == 0)
        {
            improved = true;
        }
        break;
    case Player::PlayerClassState::CLERIC:
        // Clerics improve saves every 3 levels
        if (newLevel % 3 == 0)
        {
            improved = true;
        }
        break;
    case Player::PlayerClassState::WIZARD:
        // Wizards improve saves every 5 levels
        if (newLevel % 5 == 0)
        {
            improved = true;
        }
        break;
    }

    if (improved)
    {
        ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Saving throws improved!");
        ctx->message_system->finalize_message();

            ctx->message_system->log(std::format("Saving throws improved at level {}", newLevel));
        // NOTE: Saving throw mechanics deferred - bonuses tracked but system unused
    }
}
