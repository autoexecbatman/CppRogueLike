// LevelUpSystem.cpp - Handles combat improvements on level up according to AD&D 2e rules

#include "LevelUpSystem.h"
#include "../Game.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Player.h"
#include "../dnd_tables/CalculatedTHAC0s.h"
#include "../Colors/Colors.h"
#include <format>

void LevelUpSystem::apply_level_up_benefits(Creature& owner, int newLevel)
{
    // Get the player class
    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr)
    {
        return; // Only handle player level ups
    }
    
    // Store old values for comparison in display
    int oldHP = owner.destructible->hp;
    int oldMaxHP = owner.destructible->hpMax;
    int oldTHAC0 = owner.destructible->thaco;
    
    // Apply THAC0 improvement
    apply_thac0_improvement(owner, newLevel);
    
    // Apply hit point gains
    int hpGained = apply_hit_point_gain(owner, newLevel);
    
    // Apply class-specific combat improvements
    apply_class_specific_improvements(owner, newLevel);
    
    // Apply ability score improvements (AD&D 2e: every 4 levels)
    if (newLevel % 4 == 0)
    {
        apply_ability_score_improvement(owner, newLevel);
    }
    
    // Apply saving throw improvements
    apply_saving_throw_improvements(owner, newLevel);
    
    // Log the level up with summary
    game.append_message_part(YELLOW_BLACK_PAIR, "LEVEL UP! ");
    game.append_message_part(WHITE_BLACK_PAIR, std::format("You are now level {}. ", newLevel));
    game.append_message_part(GREEN_BLACK_PAIR, std::format("+{} HP, ", hpGained));
    if (oldTHAC0 != owner.destructible->thaco)
    {
        game.append_message_part(GREEN_BLACK_PAIR, std::format("THAC0 {}->{}", oldTHAC0, owner.destructible->thaco));
    }
    game.finalize_message();
    
    game.log(std::format("Level {} reached! Combat abilities improved.", newLevel));
}

void LevelUpSystem::apply_thac0_improvement(Creature& owner, int newLevel)
{
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
        newTHAC0 = thac0Tables.getFighter(newLevel);
        break;
    case Player::PlayerClassState::ROGUE:
        newTHAC0 = thac0Tables.getRogue(newLevel);
        break;
    case Player::PlayerClassState::CLERIC:
        newTHAC0 = thac0Tables.getCleric(newLevel);
        break;
    case Player::PlayerClassState::WIZARD:
        newTHAC0 = thac0Tables.getWizard(newLevel);
        break;
    }
    
    // Only update if THAC0 improved (lower is better)
    if (newTHAC0 < owner.destructible->thaco)
    {
        int oldTHAC0 = owner.destructible->thaco;
        owner.destructible->thaco = newTHAC0;
        
        game.append_message_part(GREEN_BLACK_PAIR, "THAC0 improved");
        game.append_message_part(WHITE_BLACK_PAIR, " from ");
        game.append_message_part(WHITE_BLACK_PAIR, std::to_string(oldTHAC0));
        game.append_message_part(WHITE_BLACK_PAIR, " to ");
        game.append_message_part(GREEN_BLACK_PAIR, std::to_string(newTHAC0));
        game.append_message_part(WHITE_BLACK_PAIR, "!");
        game.finalize_message();
        
        game.log(std::format("THAC0 improved: {} -> {}", oldTHAC0, newTHAC0));
    }
}

int LevelUpSystem::apply_hit_point_gain(Creature& owner, int newLevel)
{
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
        hitDiceRoll = game.d.d10();
        diceType = "d10";
        break;
    case Player::PlayerClassState::ROGUE:
        hitDiceRoll = game.d.d6();
        diceType = "d6";
        break;
    case Player::PlayerClassState::CLERIC:
        hitDiceRoll = game.d.d8();
        diceType = "d8";
        break;
    case Player::PlayerClassState::WIZARD:
        hitDiceRoll = game.d.d4();
        diceType = "d4";
        break;
    }
    
    // Get Constitution bonus
    int conBonus = 0;
    if (owner.constitution >= 1 && owner.constitution <= game.constitutionAttributes.size())
    {
        conBonus = game.constitutionAttributes[owner.constitution - 1].HPAdj;
    }
    
    // Calculate total HP gain (minimum 1)
    int totalHPGain = std::max(1, hitDiceRoll + conBonus);
    
    // Update HP values
    owner.destructible->hpBase += hitDiceRoll; // Base HP without Con bonus
    owner.destructible->hpMax += totalHPGain;
    owner.destructible->hp += totalHPGain; // Give full HP on level up
    
    // Display HP gain message
    game.append_message_part(GREEN_BLACK_PAIR, "Hit Points increased");
    game.append_message_part(WHITE_BLACK_PAIR, " by ");
    game.append_message_part(GREEN_BLACK_PAIR, std::to_string(totalHPGain));
    game.append_message_part(WHITE_BLACK_PAIR, " (");
    game.append_message_part(WHITE_BLACK_PAIR, diceType);
    game.append_message_part(WHITE_BLACK_PAIR, ": ");
    game.append_message_part(YELLOW_BLACK_PAIR, std::to_string(hitDiceRoll));
    if (conBonus != 0)
    {
        game.append_message_part(WHITE_BLACK_PAIR, " + ");
        game.append_message_part(YELLOW_BLACK_PAIR, std::to_string(conBonus));
        game.append_message_part(WHITE_BLACK_PAIR, " CON");
    }
    game.append_message_part(WHITE_BLACK_PAIR, ")");
    game.finalize_message();
    
    game.log(std::format("HP increased by {} ({} rolled + {} CON bonus). Max HP now: {}", 
                         totalHPGain, hitDiceRoll, conBonus, owner.destructible->hpMax));
    
    return totalHPGain; // Return HP gained for display
}

void LevelUpSystem::apply_class_specific_improvements(Creature& owner, int newLevel)
{
    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr)
    {
        return;
    }
    
    switch (playerPtr->playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        apply_fighter_improvements(owner, newLevel);
        break;
    case Player::PlayerClassState::ROGUE:
        apply_rogue_improvements(owner, newLevel);
        break;
    case Player::PlayerClassState::CLERIC:
        apply_cleric_improvements(owner, newLevel);
        break;
    case Player::PlayerClassState::WIZARD:
        apply_wizard_improvements(owner, newLevel);
        break;
    }
}

void LevelUpSystem::apply_fighter_improvements(Creature& owner, int newLevel)
{
    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr)
    {
        return;
    }

    // Fighters get extra attacks at levels 7 and 13
    if (newLevel == 7)
    {
        playerPtr->attacksPerRound = 1.5f; // 3/2 attacks per round
        game.append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        game.append_message_part(GREEN_BLACK_PAIR, "Extra Attack!");
        game.append_message_part(WHITE_BLACK_PAIR, " You can now attack 3/2 times per round.");
        game.finalize_message();
        game.log("Fighter gained extra attack (3/2 attacks per round)");
    }
    else if (newLevel == 13)
    {
        playerPtr->attacksPerRound = 2.0f; // 2 attacks per round
        game.append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        game.append_message_part(GREEN_BLACK_PAIR, "Extra Attack!");
        game.append_message_part(WHITE_BLACK_PAIR, " You can now attack 2 times per round.");
        game.finalize_message();
        game.log("Fighter gained extra attack (2 attacks per round)");
    }

	// Fighters also get better weapon specialization bonuses
	if (newLevel % 3 == 0) // Every 3 levels
	{
		game.append_message_part(WHITE_BLACK_PAIR, "Your martial prowess improves!");
		game.finalize_message();
	}
}

void LevelUpSystem::apply_rogue_improvements(Creature& owner, int newLevel)
{
    // Rogues get improved backstab multipliers
    int backstabMultiplier = calculate_backstab_multiplier(newLevel);
    if (backstabMultiplier > calculate_backstab_multiplier(newLevel - 1))
    {
        game.append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        game.append_message_part(GREEN_BLACK_PAIR, "Backstab improved!");
        game.append_message_part(WHITE_BLACK_PAIR, " Damage multiplier: x");
        game.append_message_part(GREEN_BLACK_PAIR, std::to_string(backstabMultiplier));
        game.finalize_message();
        game.log(std::format("Rogue backstab multiplier increased to x{}", backstabMultiplier));
    }
    
    // Rogue skills improve
    if (newLevel % 2 == 0) // Every 2 levels
    {
        game.append_message_part(WHITE_BLACK_PAIR, "Your thieving skills improve!");
        game.finalize_message();
        // TODO: Implement thieving skills (hide in shadows, move silently, etc.)
    }
}

void LevelUpSystem::apply_cleric_improvements(Creature& owner, int newLevel)
{
    // Clerics get turn undead improvements and spell slots
    if (newLevel == 3 || newLevel == 5 || newLevel == 7 || newLevel == 9)
    {
        game.append_message_part(YELLOW_BLACK_PAIR, "Special: ");
        game.append_message_part(GREEN_BLACK_PAIR, "Turn Undead improved!");
        game.append_message_part(WHITE_BLACK_PAIR, " You can affect more powerful undead.");
        game.finalize_message();
        game.log(std::format("Cleric turn undead ability improved at level {}", newLevel));
        // TODO: Implement turn undead mechanic
    }
    
    // Spell slots increase
    if (newLevel >= 2)
    {
        game.append_message_part(WHITE_BLACK_PAIR, "Your divine power grows stronger!");
        game.finalize_message();
        // TODO: Implement spell system
    }
}

void LevelUpSystem::apply_wizard_improvements(Creature& owner, int newLevel)
{
    // Wizards primarily get more spell slots and access to higher level spells
    if (newLevel % 2 == 1 && newLevel > 1) // Odd levels after 1st
    {
        int spellLevel = (newLevel + 1) / 2;
        if (spellLevel <= 9) // Max spell level is 9
        {
            game.append_message_part(YELLOW_BLACK_PAIR, "Special: ");
            game.append_message_part(GREEN_BLACK_PAIR, "New spell level!");
            game.append_message_part(WHITE_BLACK_PAIR, " You can now cast level ");
            game.append_message_part(GREEN_BLACK_PAIR, std::to_string(spellLevel));
            game.append_message_part(WHITE_BLACK_PAIR, " spells.");
            game.finalize_message();
            game.log(std::format("Wizard can now cast level {} spells", spellLevel));
        }
    }
    
    // Wizards get better spell power
    game.append_message_part(WHITE_BLACK_PAIR, "Your arcane knowledge deepens!");
    game.finalize_message();
    // TODO: Implement spell system
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

void LevelUpSystem::apply_ability_score_improvement(Creature& owner, int newLevel)
{
    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr)
    {
        return;
    }
    
    // AD&D 2e: Players can increase ability scores every 4 levels
    game.append_message_part(YELLOW_BLACK_PAIR, "Special: ");
    game.append_message_part(GREEN_BLACK_PAIR, "Ability Score Improvement!");
    game.append_message_part(WHITE_BLACK_PAIR, " You may increase one ability score by 1 point.");
    game.finalize_message();
    
    // TODO: Implement interactive ability score selection
    // For now, automatically improve the prime requisite for the class
    switch (playerPtr->playerClassState)
    {
    case Player::PlayerClassState::FIGHTER:
        playerPtr->strength = std::min(18, playerPtr->strength + 1);
        game.append_message_part(GREEN_BLACK_PAIR, "Strength increased to ");
        game.append_message_part(GREEN_BLACK_PAIR, std::to_string(playerPtr->strength));
        game.append_message_part(WHITE_BLACK_PAIR, "!");
        game.finalize_message();
        break;
    case Player::PlayerClassState::ROGUE:
        playerPtr->dexterity = std::min(18, playerPtr->dexterity + 1);
        game.append_message_part(GREEN_BLACK_PAIR, "Dexterity increased to ");
        game.append_message_part(GREEN_BLACK_PAIR, std::to_string(playerPtr->dexterity));
        game.append_message_part(WHITE_BLACK_PAIR, "!");
        game.finalize_message();
        break;
    case Player::PlayerClassState::CLERIC:
        playerPtr->wisdom = std::min(18, playerPtr->wisdom + 1);
        game.append_message_part(GREEN_BLACK_PAIR, "Wisdom increased to ");
        game.append_message_part(GREEN_BLACK_PAIR, std::to_string(playerPtr->wisdom));
        game.append_message_part(WHITE_BLACK_PAIR, "!");
        game.finalize_message();
        break;
    case Player::PlayerClassState::WIZARD:
        playerPtr->intelligence = std::min(18, playerPtr->intelligence + 1);
        game.append_message_part(GREEN_BLACK_PAIR, "Intelligence increased to ");
        game.append_message_part(GREEN_BLACK_PAIR, std::to_string(playerPtr->intelligence));
        game.append_message_part(WHITE_BLACK_PAIR, "!");
        game.finalize_message();
        break;
    }
    
    game.log(std::format("Ability score improved at level {}", newLevel));
}

void LevelUpSystem::apply_saving_throw_improvements(Creature& owner, int newLevel)
{
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
        game.append_message_part(WHITE_BLACK_PAIR, "Saving throws improved!");
        game.finalize_message();
        game.log(std::format("Saving throws improved at level {}", newLevel));
        // TODO: Implement actual saving throw system
    }
}
