// file: Attacker.cpp
#include <iostream>
#include <string>
#include <format>
#include <memory>

#include "../Game.h"
#include "../Colors/Colors.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Monsters.h"
#include "../ActorTypes/Player.h"
#include "../Menu/MenuTrade.h"
#include "Attacker.h"

Attacker::Attacker(std::string roll) : roll(roll) {}

// Now modify the attack function in Attacker.cpp to incorporate the surprise check:
void Attacker::attack(Creature& attacker, Creature& target)
{
    // if target is shopkeeper, do not attack
    if (target.actorData.name == "shopkeeper" && !attacker.has_state(ActorState::IS_RANGED))
    {
        game.menus.push_back(std::make_unique<MenuTrade>(target, attacker));
        return;
    }

    if (!target.destructible->is_dead() && attacker.strength > 0) // if target is not dead and attacker has strength
    {
        StrengthAttributes strength = game.strengthAttributes.at(attacker.strength - 1); // get the strength attributes for the attacker

        // roll for attack and damage
        int rollAttack = game.d.d20();
        int rollDmg = game.d.roll_from_string(roll);

        // THAC0 calculation
        int rollNeeded = attacker.destructible->thaco - target.destructible->armorClass;

        // Apply dexterity missile attack adjustment if this is a ranged attack
        int hitModifier = 0;
        if (attacker.has_state(ActorState::IS_RANGED))
        {
            // Get dexterity bonus for missile attacks
            auto& dexAttributes = game.dexterityAttributes.at(attacker.dexterity - 1);
            hitModifier = dexAttributes.MissileAttackAdj;

            // Display missile attack bonus info
            if (hitModifier != 0) {
                game.log("Applying ranged attack modifier: " + std::to_string(hitModifier) + " from dexterity " + std::to_string(attacker.dexterity));
            }
        }

        // NOTE: Defensive adjustment from target's dexterity is now handled in update_armor_class
        // No need to apply it again here

        // Apply the hit modifier (positive is better)
        rollNeeded -= hitModifier;

        if (rollAttack >= rollNeeded) // if the attack roll is greater than or equal to the roll needed
        {
            // calculate the adjusted damage
            const int adjDmg = rollDmg + strength.dmgAdj; // add strength bonus
            const int totaldmg = adjDmg - target.destructible->dr; // substract damage reduction
            const int finalDamage = std::max(0, totaldmg); // ensure damage is not negative
            
            // Display the successful attack roll with damage
            game.appendMessagePart(attacker.actorData.color, attacker.actorData.name);
            game.appendMessagePart(WHITE_BLACK_PAIR, " rolls ");
            game.appendMessagePart(GREEN_BLACK_PAIR, std::to_string(rollAttack));
            game.appendMessagePart(WHITE_BLACK_PAIR, " roll needed ");
            game.appendMessagePart(WHITE_BLACK_PAIR, std::to_string(rollNeeded));
            game.appendMessagePart(GREEN_BLACK_PAIR, ". Hit! ");
            game.appendMessagePart(RED_BLACK_PAIR, std::to_string(finalDamage));
            game.appendMessagePart(WHITE_BLACK_PAIR, " dmg.");
            game.finalizeMessage();
            
            // Debug log the successful attack roll
            game.log("ATTACK HIT: " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) + 
                     " vs " + std::to_string(rollNeeded) + " needed (THAC0:" + std::to_string(attacker.destructible->thaco) + 
                     ", AC:" + std::to_string(target.destructible->armorClass) + ")");
            
            // Apply damage to target if any
            if (finalDamage > 0)
            {
                // Debug log the damage roll details
                game.log("DAMAGE DEALT: " + std::to_string(rollDmg) + " (base) + " + 
                         std::to_string(strength.dmgAdj) + " (str) - " + std::to_string(target.destructible->dr) + 
                         " (DR) = " + std::to_string(finalDamage) + " damage to " + target.actorData.name);
                
                // apply damage to target
                target.destructible->take_damage(target, finalDamage);
            }
            else
            {
                // Debug log the no-damage attack
                game.log("NO DAMAGE: " + std::to_string(rollDmg) + " (base) + " + 
                         std::to_string(strength.dmgAdj) + " (str) - " + std::to_string(target.destructible->dr) + 
                         " (DR) = 0 damage to " + target.actorData.name);
            }
        }
        else
        {
            // Display the failed attack roll with 0 damage
            game.appendMessagePart(attacker.actorData.color, attacker.actorData.name);
            game.appendMessagePart(WHITE_BLACK_PAIR, " rolls ");
            game.appendMessagePart(RED_BLACK_PAIR, std::to_string(rollAttack));
            game.appendMessagePart(WHITE_BLACK_PAIR, " roll needed ");
            game.appendMessagePart(WHITE_BLACK_PAIR, std::to_string(rollNeeded));
            game.appendMessagePart(RED_BLACK_PAIR, ". Miss! ");
            game.appendMessagePart(RED_BLACK_PAIR, "0");
            game.appendMessagePart(WHITE_BLACK_PAIR, " dmg.");
            game.finalizeMessage();
            
            // Debug log the failed attack roll
            game.log("ATTACK MISS: " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) + 
                     " vs " + std::to_string(rollNeeded) + " needed (THAC0:" + std::to_string(attacker.destructible->thaco) + 
                     ", AC:" + std::to_string(target.destructible->armorClass) + ")");
        }
    }
    else
    {
        game.appendMessagePart(attacker.actorData.color, std::format("{}", attacker.actorData.name));
        game.appendMessagePart(WHITE_BLACK_PAIR, std::format(" attacks "));
        game.appendMessagePart(target.actorData.color, std::format("{}", target.actorData.name));
        game.appendMessagePart(WHITE_BLACK_PAIR, std::format(" in vain."));
        game.finalizeMessage();
    }
}

void Attacker::load(const json& j)
{
	roll = j["roll"];
}

void Attacker::save(json& j)
{
	j["roll"] = roll;
}

void Attacker::attack_with_dual_wield(Creature& attacker, Creature& target)
{
	// Cast to Player to check dual wielding
	auto* player = dynamic_cast<Player*>(&attacker);
	if (!player)
	{
		// Fallback to normal attack for non-players
		attack(attacker, target);
		return;
	}
	
	// Get dual wield information
	auto dualWieldInfo = player->get_dual_wield_info();
	
	if (!dualWieldInfo.isDualWielding)
	{
		// Not dual wielding, use normal attack
		attack(attacker, target);
		return;
	}
	
	// Dual wielding - perform both attacks
	game.appendMessagePart(WHITE_BLACK_PAIR, "Dual wielding: ");
	game.appendMessagePart(GREEN_BLACK_PAIR, "Fighting with both weapons!");
	game.finalizeMessage();
	
	// Main hand attack
	perform_single_attack(attacker, target, roll, dualWieldInfo.mainHandPenalty, "main hand");
	
	// Check if target is still alive (is_dead() is safe to call even if HP <= 0)
	if (target.destructible && !target.destructible->is_dead())
	{
		// Target survived main hand attack, proceed with off-hand
		perform_single_attack(attacker, target, dualWieldInfo.offHandDamageRoll, dualWieldInfo.offHandPenalty, "off hand");
	}
	// Note: If target died, it's marked as dead but not yet removed from vector
	// Cleanup happens later in the game loop
}

void Attacker::perform_single_attack(Creature& attacker, Creature& target, const std::string& damageRoll, int attackPenalty, const std::string& handName)
{
	// Safety check: ensure target has valid destructible component
	if (!target.destructible)
	{
		game.log("WARNING: Target has no destructible component in performSingleAttack");
		return;
	}
	
	// if target is shopkeeper, do not attack
	if (target.actorData.name == "shopkeeper" && !attacker.has_state(ActorState::IS_RANGED))
	{
		game.menus.push_back(std::make_unique<MenuTrade>(target, attacker));
		return;
	}

	if (!target.destructible->is_dead() && attacker.strength > 0) // if target is not dead and attacker has strength
	{
		StrengthAttributes strength = game.strengthAttributes.at(attacker.strength - 1); // get the strength attributes for the attacker

		// roll for attack and damage
		int rollAttack = game.d.d20();
		int rollDmg = game.d.roll_from_string(damageRoll);

		// THAC0 calculation
		int rollNeeded = attacker.destructible->thaco - target.destructible->armorClass;

		// Apply dexterity missile attack adjustment if this is a ranged attack
		int hitModifier = attackPenalty; // Start with dual wield penalty
		if (attacker.has_state(ActorState::IS_RANGED))
		{
			// Get dexterity bonus for missile attacks
			auto& dexAttributes = game.dexterityAttributes.at(attacker.dexterity - 1);
			hitModifier += dexAttributes.MissileAttackAdj;

			// Display missile attack bonus info
			if (dexAttributes.MissileAttackAdj != 0) {
				game.log("Applying ranged attack modifier: " + std::to_string(dexAttributes.MissileAttackAdj) + " from dexterity " + std::to_string(attacker.dexterity));
			}
		}

		// Apply the hit modifier (positive is better, so subtract from roll needed)
		rollNeeded -= hitModifier;

		if (rollAttack >= rollNeeded) // if the attack roll is greater than or equal to the roll needed
		{
			// calculate the adjusted damage
			const int adjDmg = rollDmg + strength.dmgAdj; // add strength bonus
			const int totaldmg = adjDmg - target.destructible->dr; // substract damage reduction
			const int finalDamage = std::max(0, totaldmg); // ensure damage is not negative
			
			// Display the successful attack roll with damage
			game.appendMessagePart(attacker.actorData.color, attacker.actorData.name);
			game.appendMessagePart(WHITE_BLACK_PAIR, " (" + handName + ") rolls ");
			game.appendMessagePart(GREEN_BLACK_PAIR, std::to_string(rollAttack));
			if (attackPenalty != 0)
			{
				game.appendMessagePart(WHITE_BLACK_PAIR, " (" + std::to_string(attackPenalty) + ")");
			}
			game.appendMessagePart(WHITE_BLACK_PAIR, " vs " + std::to_string(rollNeeded));
			game.appendMessagePart(GREEN_BLACK_PAIR, ". Hit! ");
			game.appendMessagePart(RED_BLACK_PAIR, std::to_string(finalDamage));
			game.appendMessagePart(WHITE_BLACK_PAIR, " dmg.");
			game.finalizeMessage();
			
			// Debug log the successful attack roll
			game.log("ATTACK HIT (" + handName + "): " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) + 
					 " vs " + std::to_string(rollNeeded) + " needed (THAC0:" + std::to_string(attacker.destructible->thaco) + 
					 ", AC:" + std::to_string(target.destructible->armorClass) + ", Penalty:" + std::to_string(attackPenalty) + ")");
			
			// Apply damage to target if any
			if (finalDamage > 0)
			{
				// Debug log the damage roll details
				game.log("DAMAGE DEALT (" + handName + "): " + std::to_string(rollDmg) + " (base) + " + 
						 std::to_string(strength.dmgAdj) + " (str) - " + std::to_string(target.destructible->dr) + 
						 " (DR) = " + std::to_string(finalDamage) + " damage to " + target.actorData.name);
				
				// apply damage to target
				target.destructible->take_damage(target, finalDamage);
			}
			else
			{
				// Debug log the no-damage attack
				game.log("NO DAMAGE (" + handName + "): " + std::to_string(rollDmg) + " (base) + " + 
						 std::to_string(strength.dmgAdj) + " (str) - " + std::to_string(target.destructible->dr) + 
						 " (DR) = 0 damage to " + target.actorData.name);
			}
		}
		else
		{
			// Display the failed attack roll with 0 damage
			game.appendMessagePart(attacker.actorData.color, attacker.actorData.name);
			game.appendMessagePart(WHITE_BLACK_PAIR, " (" + handName + ") rolls ");
			game.appendMessagePart(RED_BLACK_PAIR, std::to_string(rollAttack));
			if (attackPenalty != 0)
			{
				game.appendMessagePart(WHITE_BLACK_PAIR, " (" + std::to_string(attackPenalty) + ")");
			}
			game.appendMessagePart(WHITE_BLACK_PAIR, " vs " + std::to_string(rollNeeded));
			game.appendMessagePart(RED_BLACK_PAIR, ". Miss! ");
			game.appendMessagePart(RED_BLACK_PAIR, "0");
			game.appendMessagePart(WHITE_BLACK_PAIR, " dmg.");
			game.finalizeMessage();
			
			// Debug log the failed attack roll
			game.log("ATTACK MISS (" + handName + "): " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) + 
					 " vs " + std::to_string(rollNeeded) + " needed (THAC0:" + std::to_string(attacker.destructible->thaco) + 
					 ", AC:" + std::to_string(target.destructible->armorClass) + ", Penalty:" + std::to_string(attackPenalty) + ")");
		}
	}
	else
	{
		game.appendMessagePart(attacker.actorData.color, std::format("{}", attacker.actorData.name));
		game.appendMessagePart(WHITE_BLACK_PAIR, std::format(" attacks "));
		game.appendMessagePart(target.actorData.color, std::format("{}", target.actorData.name));
		game.appendMessagePart(WHITE_BLACK_PAIR, std::format(" in vain."));
		game.finalizeMessage();
	}
}

// end of file: Attacker.cpp
