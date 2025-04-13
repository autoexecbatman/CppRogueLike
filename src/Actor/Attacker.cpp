// file: Attacker.cpp
#include <iostream>
#include <string>
#include <gsl/gsl>
#include <gsl/util>
#include <format>
#include <memory>

#include "../Game.h"
#include "../Colors/Colors.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/Monsters.h"
#include "../MenuTrade.h"
#include "Attacker.h"

Attacker::Attacker(std::string roll) : roll(roll) {}

bool Attacker::checkSurprise(Creature& attacker, Creature& target)
{
    // Each side rolls 2d6 + reaction adjustment based on dexterity
    int attackerDex = attacker.dexterity;
    int targetDex = target.dexterity;

    // Get reaction adjustment from dexterity attributes
    int attackerReactionAdj = 0;
    int targetReactionAdj = 0;

    // Look up reaction adjustment if dexterity is in valid range
    if (attackerDex > 0 && attackerDex <= game.dexterityAttributes.size()) {
        attackerReactionAdj = game.dexterityAttributes[attackerDex - 1].ReactionAdj;
    }

    if (targetDex > 0 && targetDex <= game.dexterityAttributes.size()) {
        targetReactionAdj = game.dexterityAttributes[targetDex - 1].ReactionAdj;
    }

    // Roll 2d6 + reaction adjustment for each
    int attackerRoll = game.d.d6() + game.d.d6() + attackerReactionAdj;
    int targetRoll = game.d.d6() + game.d.d6() + targetReactionAdj;

    // Log the rolls for debugging
    game.log("Surprise check: " + attacker.actorData.name + " rolled " +
        std::to_string(attackerRoll) + " vs " + target.actorData.name +
        " rolled " + std::to_string(targetRoll));

    // Return true if attacker wins the roll
    return attackerRoll > targetRoll;
}

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

        bool surpriseSuccess = false;

        surpriseSuccess = checkSurprise(attacker, target);

        if (surpriseSuccess)
        {
            // Display surprise message
            game.appendMessagePart(attacker.actorData.color, attacker.actorData.name);
            game.appendMessagePart(WHITE_PAIR, " catches ");
            game.appendMessagePart(target.actorData.color, target.actorData.name);
            game.appendMessagePart(WHITE_PAIR, " by surprise!");
            game.finalizeMessage();
        }

        // roll for attack and damage
        int rollAttack = game.d.d20();
        int rollDmg = game.d.roll_from_string(roll);

        // THAC0 calculation
        int rollNeeded = attacker.destructible->thaco - target.destructible->armorClass;

        // Apply surprise bonus if successful (+1 to hit)
        if (surpriseSuccess)
        {
            rollAttack += 1;
            game.log("Surprise attack bonus applied: -1 to hit required");
        }

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

            // if damage is dealt display combat messages
            if (totaldmg > 0)
            {
                game.appendMessagePart(attacker.actorData.color, std::format("{}", attacker.actorData.name));
                game.appendMessagePart(WHITE_PAIR, " attacks the ");
                game.appendMessagePart(target.actorData.color, std::format("{}", target.actorData.name));

                // Indicate if this was a ranged attack
                if (attacker.has_state(ActorState::IS_RANGED)) {
                    game.appendMessagePart(WHITE_PAIR, std::format(" from a distance"));
                }

                game.appendMessagePart(WHITE_PAIR, std::format(" for {} hit points.", totaldmg));
                game.finalizeMessage();
                // apply damage to target
                target.destructible->take_damage(target, totaldmg);
            }
            // else no damage message
            else
            {
                game.appendMessagePart(attacker.actorData.color, std::format("{}", attacker.actorData.name));
                game.appendMessagePart(WHITE_PAIR, std::format(" attacks "));
                game.appendMessagePart(target.actorData.color, std::format("{}", target.actorData.name));
                game.appendMessagePart(WHITE_PAIR, std::format(" but it has no effect!"));
                game.finalizeMessage();
            }
        }
        else
        {
            game.appendMessagePart(attacker.actorData.color, std::format("{}", attacker.actorData.name));
            game.appendMessagePart(WHITE_PAIR, std::format(" attacks "));
            game.appendMessagePart(target.actorData.color, std::format("{}", target.actorData.name));
            game.appendMessagePart(WHITE_PAIR, std::format(" and misses."));
            game.finalizeMessage();
        }
    }
    else
    {
        game.appendMessagePart(attacker.actorData.color, std::format("{}", attacker.actorData.name));
        game.appendMessagePart(WHITE_PAIR, std::format(" attacks "));
        game.appendMessagePart(target.actorData.color, std::format("{}", target.actorData.name));
        game.appendMessagePart(WHITE_PAIR, std::format(" in vain."));
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

// end of file: Attacker.cpp
