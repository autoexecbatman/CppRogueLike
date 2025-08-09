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
    
    // Apply THAC0 improvement
    apply_thac0_improvement(owner, newLevel);
    
    // Apply hit point gains
    apply_hit_point_gain(owner, newLevel);
    
    // Apply class-specific combat improvements
    apply_class_specific_improvements(owner, newLevel);
    
    // Log the level up
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
        
        game.appendMessagePart(GREEN_BLACK_PAIR, "THAC0 improved");
        game.appendMessagePart(WHITE_BLACK_PAIR, " from ");
        game.appendMessagePart(WHITE_BLACK_PAIR, std::to_string(oldTHAC0));
        game.appendMessagePart(WHITE_BLACK_PAIR, " to ");
        game.appendMessagePart(GREEN_BLACK_PAIR, std::to_string(newTHAC0));
        game.appendMessagePart(WHITE_BLACK_PAIR, "!");
        game.finalizeMessage();
        
        game.log(std::format("THAC0 improved: {} -> {}", oldTHAC0, newTHAC0));
    }
}

void LevelUpSystem::apply_hit_point_gain(Creature& owner, int newLevel)
{
    auto playerPtr = dynamic_cast<Player*>(&owner);
    if (!playerPtr || !owner.destructible)
    {
        return;
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
    game.appendMessagePart(GREEN_BLACK_PAIR, "Hit Points increased");
    game.appendMessagePart(WHITE_BLACK_PAIR, " by ");
    game.appendMessagePart(GREEN_BLACK_PAIR, std::to_string(totalHPGain));
    game.appendMessagePart(WHITE_BLACK_PAIR, " (");
    game.appendMessagePart(WHITE_BLACK_PAIR, diceType);
    game.appendMessagePart(WHITE_BLACK_PAIR, ": ");
    game.appendMessagePart(YELLOW_BLACK_PAIR, std::to_string(hitDiceRoll));
    if (conBonus != 0)
    {
        game.appendMessagePart(WHITE_BLACK_PAIR, " + ");
        game.appendMessagePart(YELLOW_BLACK_PAIR, std::to_string(conBonus));
        game.appendMessagePart(WHITE_BLACK_PAIR, " CON");
    }
    game.appendMessagePart(WHITE_BLACK_PAIR, ")");
    game.finalizeMessage();
    
    game.log(std::format("HP increased by {} ({} rolled + {} CON bonus). Max HP now: {}", 
                         totalHPGain, hitDiceRoll, conBonus, owner.destructible->hpMax));
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
 game.appendMessagePart(YELLOW_BLACK_PAIR, "Special: ");
 game.appendMessagePart(GREEN_BLACK_PAIR, "Extra Attack!");
game.appendMessagePart(WHITE_BLACK_PAIR, " You can now attack 3/2 times per round.");
game.finalizeMessage();
game.log("Fighter gained extra attack (3/2 attacks per round)");
}
else if (newLevel == 13)
{
 playerPtr->attacksPerRound = 2.0f; // 2 attacks per round
 game.appendMessagePart(YELLOW_BLACK_PAIR, "Special: ");
 game.appendMessagePart(GREEN_BLACK_PAIR, "Extra Attack!");
 game.appendMessagePart(WHITE_BLACK_PAIR, " You can now attack 2 times per round.");
 game.finalizeMessage();
game.log("Fighter gained extra attack (2 attacks per round)");
}

	// Fighters also get better weapon specialization bonuses
	if (newLevel % 3 == 0) // Every 3 levels
	{
		game.appendMessagePart(WHITE_BLACK_PAIR, "Your martial prowess improves!");
		game.finalizeMessage();
	}
}

void LevelUpSystem::apply_rogue_improvements(Creature& owner, int newLevel)
{
    // Rogues get improved backstab multipliers
    int backstabMultiplier = calculate_backstab_multiplier(newLevel);
    if (backstabMultiplier > calculate_backstab_multiplier(newLevel - 1))
    {
        game.appendMessagePart(YELLOW_BLACK_PAIR, "Special: ");
        game.appendMessagePart(GREEN_BLACK_PAIR, "Backstab improved!");
        game.appendMessagePart(WHITE_BLACK_PAIR, " Damage multiplier: x");
        game.appendMessagePart(GREEN_BLACK_PAIR, std::to_string(backstabMultiplier));
        game.finalizeMessage();
        game.log(std::format("Rogue backstab multiplier increased to x{}", backstabMultiplier));
    }
    
    // Rogue skills improve
    if (newLevel % 2 == 0) // Every 2 levels
    {
        game.appendMessagePart(WHITE_BLACK_PAIR, "Your thieving skills improve!");
        game.finalizeMessage();
        // TODO: Implement thieving skills (hide in shadows, move silently, etc.)
    }
}

void LevelUpSystem::apply_cleric_improvements(Creature& owner, int newLevel)
{
    // Clerics get turn undead improvements and spell slots
    if (newLevel == 3 || newLevel == 5 || newLevel == 7 || newLevel == 9)
    {
        game.appendMessagePart(YELLOW_BLACK_PAIR, "Special: ");
        game.appendMessagePart(GREEN_BLACK_PAIR, "Turn Undead improved!");
        game.appendMessagePart(WHITE_BLACK_PAIR, " You can affect more powerful undead.");
        game.finalizeMessage();
        game.log(std::format("Cleric turn undead ability improved at level {}", newLevel));
        // TODO: Implement turn undead mechanic
    }
    
    // Spell slots increase
    if (newLevel >= 2)
    {
        game.appendMessagePart(WHITE_BLACK_PAIR, "Your divine power grows stronger!");
        game.finalizeMessage();
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
            game.appendMessagePart(YELLOW_BLACK_PAIR, "Special: ");
            game.appendMessagePart(GREEN_BLACK_PAIR, "New spell level!");
            game.appendMessagePart(WHITE_BLACK_PAIR, " You can now cast level ");
            game.appendMessagePart(GREEN_BLACK_PAIR, std::to_string(spellLevel));
            game.appendMessagePart(WHITE_BLACK_PAIR, " spells.");
            game.finalizeMessage();
            game.log(std::format("Wizard can now cast level {} spells", spellLevel));
        }
    }
    
    // Wizards get better spell power
    game.appendMessagePart(WHITE_BLACK_PAIR, "Your arcane knowledge deepens!");
    game.finalizeMessage();
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
