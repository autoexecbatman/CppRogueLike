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
#include "../Ai/AiShopkeeper.h"

Attacker::Attacker(std::string roll) : roll(roll), damageInfo(parse_damage_from_roll_string(roll)) {}

Attacker::Attacker(const DamageInfo& damage) : damageInfo(damage), roll(damage.displayRoll) {}

// Now modify the attack function in Attacker.cpp to incorporate the surprise check:
void Attacker::attack(Creature& attacker, Creature& target)
{
    // if target is shopkeeper, do not attack
    if (dynamic_cast<AiShopkeeper*>(target.ai.get()) && !attacker.has_state(ActorState::IS_RANGED))
    {
        game.menus.push_back(std::make_unique<MenuTrade>(target, attacker));
        return;
    }

    if (!target.destructible->is_dead() && attacker.get_strength() > 0)
    {
        const auto& strength = game.data_manager.get_strength_attributes().at(attacker.get_strength() - 1);

        // Roll for attack and damage using enhanced DamageInfo system
        int rollAttack = game.d.d20();

        // Get unified damage using clean interface
        DamageInfo attackDamage = get_attack_damage(attacker);
        int rollDmg = attackDamage.roll_damage();

        // THAC0 calculation
        int rollNeeded = attacker.destructible->get_thaco() - target.destructible->get_armor_class();

        // Apply dexterity missile attack adjustment if this is a ranged attack
        int hitModifier = 0;
        if (attacker.has_state(ActorState::IS_RANGED))
        {
            const auto& dexAttributes = game.data_manager.get_dexterity_attributes().at(attacker.get_dexterity() - 1);
            hitModifier = dexAttributes.MissileAttackAdj;

            if (hitModifier != 0) {
                game.log("Applying ranged attack modifier: " + std::to_string(hitModifier) + " from dexterity " + std::to_string(attacker.get_dexterity()));
            }
        }

        rollNeeded -= hitModifier;

        if (rollAttack >= rollNeeded)
        {
            // Calculate damage with proper bounds checking
            const int adjDmg = rollDmg + strength.dmgAdj;
            const int totaldmg = adjDmg - target.destructible->get_dr();
            const int finalDamage = std::max(0, totaldmg);
            
            // Display attack with proper damage range info
            game.append_message_part(attacker.actorData.color, attacker.actorData.name);
            game.append_message_part(WHITE_BLACK_PAIR, " rolls ");
            game.append_message_part(GREEN_BLACK_PAIR, std::to_string(rollAttack));
            game.append_message_part(WHITE_BLACK_PAIR, " roll needed ");
            game.append_message_part(WHITE_BLACK_PAIR, std::to_string(rollNeeded));
            game.append_message_part(GREEN_BLACK_PAIR, ". Hit! ");
            game.append_message_part(RED_BLACK_PAIR, std::to_string(finalDamage));
            game.append_message_part(WHITE_BLACK_PAIR, " dmg (" + attackDamage.displayRoll + ").");
            game.finalize_message();
            
            // Enhanced debug logging with damage range
            game.log("ATTACK HIT: " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) +
                     " vs " + std::to_string(rollNeeded) + " needed | Damage: " + std::to_string(rollDmg) +
                     " (" + attackDamage.get_damage_range() + ") + " + std::to_string(strength.dmgAdj) +
                     " str - " + std::to_string(target.destructible->get_dr()) + " DR = " + std::to_string(finalDamage));
            
            if (finalDamage > 0)
            {
                target.destructible->take_damage(target, finalDamage);
            }
        }
        else
        {
            // Miss display
            game.append_message_part(attacker.actorData.color, attacker.actorData.name);
            game.append_message_part(WHITE_BLACK_PAIR, " rolls ");
            game.append_message_part(RED_BLACK_PAIR, std::to_string(rollAttack));
            game.append_message_part(WHITE_BLACK_PAIR, " roll needed ");
            game.append_message_part(WHITE_BLACK_PAIR, std::to_string(rollNeeded));
            game.append_message_part(RED_BLACK_PAIR, ". Miss!");
            game.finalize_message();
            
            game.log("ATTACK MISS: " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) + 
                     " vs " + std::to_string(rollNeeded) + " needed");
        }
    }
    else
    {
        game.append_message_part(attacker.actorData.color, std::format("{}", attacker.actorData.name));
        game.append_message_part(WHITE_BLACK_PAIR, std::format(" attacks "));
        game.append_message_part(target.actorData.color, std::format("{}", target.actorData.name));
        game.append_message_part(WHITE_BLACK_PAIR, std::format(" in vain."));
        game.finalize_message();
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
	game.append_message_part(WHITE_BLACK_PAIR, "Dual wielding: ");
	game.append_message_part(GREEN_BLACK_PAIR, "Fighting with both weapons!");
	game.finalize_message();
	
	// Main hand attack using clean interface
	perform_single_attack(attacker, target, dualWieldInfo.mainHandPenalty, "main hand");

	// Check if target is still alive (is_dead() is safe to call even if HP <= 0)
	if (target.destructible && !target.destructible->is_dead())
	{
		// Target survived main hand attack, proceed with off-hand using clean interface
		perform_single_attack(attacker, target, dualWieldInfo.offHandPenalty, "off hand");
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
	if (dynamic_cast<AiShopkeeper*>(target.ai.get()) && !attacker.has_state(ActorState::IS_RANGED))
	{
		game.menus.push_back(std::make_unique<MenuTrade>(target, attacker));
		return;
	}

	if (!target.destructible->is_dead() && attacker.get_strength() > 0) // if target is not dead and attacker has strength
	{
		const auto& strength = game.data_manager.get_strength_attributes().at(attacker.get_strength() - 1); // get the strength attributes for the attacker

		// Get proper damage info for dual wield attacks
		DamageInfo weaponDamage = parse_damage_from_roll_string(damageRoll);

		// roll for attack and damage
		int rollAttack = game.d.d20();
		int rollDmg = weaponDamage.roll_damage();

		// THAC0 calculation
		int rollNeeded = attacker.destructible->get_thaco() - target.destructible->get_armor_class();

		// Apply dexterity missile attack adjustment if this is a ranged attack
		int hitModifier = attackPenalty; // Start with dual wield penalty
		if (attacker.has_state(ActorState::IS_RANGED))
		{
			// Get dexterity bonus for missile attacks
			const auto& dexAttributes = game.data_manager.get_dexterity_attributes().at(attacker.get_dexterity() - 1);
			hitModifier += dexAttributes.MissileAttackAdj;

			// Display missile attack bonus info
			if (dexAttributes.MissileAttackAdj != 0) {
				game.log("Applying ranged attack modifier: " + std::to_string(dexAttributes.MissileAttackAdj) + " from dexterity " + std::to_string(attacker.get_dexterity()));
			}
		}

		// Apply the hit modifier (positive is better, so subtract from roll needed)
		rollNeeded -= hitModifier;

		if (rollAttack >= rollNeeded) // if the attack roll is greater than or equal to the roll needed
		{
			// calculate the adjusted damage
			const int adjDmg = rollDmg + strength.dmgAdj; // add strength bonus
			const int totaldmg = adjDmg - target.destructible->get_dr(); // substract damage reduction
			const int finalDamage = std::max(0, totaldmg); // ensure damage is not negative
			
			// Display the successful attack roll with damage
			game.append_message_part(attacker.actorData.color, attacker.actorData.name);
			game.append_message_part(WHITE_BLACK_PAIR, " (" + handName + ") rolls ");
			game.append_message_part(GREEN_BLACK_PAIR, std::to_string(rollAttack));
			if (attackPenalty != 0)
			{
				game.append_message_part(WHITE_BLACK_PAIR, " (" + std::to_string(attackPenalty) + ")");
			}
			game.append_message_part(WHITE_BLACK_PAIR, " vs " + std::to_string(rollNeeded));
			game.append_message_part(GREEN_BLACK_PAIR, ". Hit! ");
			game.append_message_part(RED_BLACK_PAIR, std::to_string(finalDamage));
			game.append_message_part(WHITE_BLACK_PAIR, " dmg (" + weaponDamage.displayRoll + ").");
			game.finalize_message();
			
			// Enhanced debug logging with damage range
			game.log("ATTACK HIT (" + handName + "): " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) + 
					 " vs " + std::to_string(rollNeeded) + " needed | Damage: " + std::to_string(rollDmg) + 
					 " (" + weaponDamage.get_damage_range() + ") + " + std::to_string(strength.dmgAdj) + 
					 " str - " + std::to_string(target.destructible->get_dr()) + " DR = " + std::to_string(finalDamage));
			
			// Apply damage to target if any
			if (finalDamage > 0)
			{
				// Debug log the damage roll details
				game.log("DAMAGE DEALT (" + handName + "): " + std::to_string(rollDmg) + " (base) + " + 
						 std::to_string(strength.dmgAdj) + " (str) - " + std::to_string(target.destructible->get_dr()) + 
						 " (DR) = " + std::to_string(finalDamage) + " damage to " + target.actorData.name);
				
				// apply damage to target
				target.destructible->take_damage(target, finalDamage);
			}
			else
			{
				// Debug log the no-damage attack
				game.log("NO DAMAGE (" + handName + "): " + std::to_string(rollDmg) + " (base) + " + 
						 std::to_string(strength.dmgAdj) + " (str) - " + std::to_string(target.destructible->get_dr()) + 
						 " (DR) = 0 damage to " + target.actorData.name);
			}
		}
		else
		{
			// Display the failed attack roll with 0 damage
			game.append_message_part(attacker.actorData.color, attacker.actorData.name);
			game.append_message_part(WHITE_BLACK_PAIR, " (" + handName + ") rolls ");
			game.append_message_part(RED_BLACK_PAIR, std::to_string(rollAttack));
			if (attackPenalty != 0)
			{
				game.append_message_part(WHITE_BLACK_PAIR, " (" + std::to_string(attackPenalty) + ")");
			}
			game.append_message_part(WHITE_BLACK_PAIR, " vs " + std::to_string(rollNeeded));
			game.append_message_part(RED_BLACK_PAIR, ". Miss! ");
			game.append_message_part(RED_BLACK_PAIR, "0");
			game.append_message_part(WHITE_BLACK_PAIR, " dmg.");
			game.finalize_message();
			
			// Debug log the failed attack roll
			game.log("ATTACK MISS (" + handName + "): " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) + 
					 " vs " + std::to_string(rollNeeded) + " needed (THAC0:" + std::to_string(attacker.destructible->get_thaco()) + 
					 ", AC:" + std::to_string(target.destructible->get_armor_class()) + ", Penalty:" + std::to_string(attackPenalty) + ")");
		}
	}
	else
	{
		game.append_message_part(attacker.actorData.color, std::format("{}", attacker.actorData.name));
		game.append_message_part(WHITE_BLACK_PAIR, std::format(" attacks "));
		game.append_message_part(target.actorData.color, std::format("{}", target.actorData.name));
		game.append_message_part(WHITE_BLACK_PAIR, std::format(" in vain."));
		game.finalize_message();
	}
}

// Modern clean interface for single attacks
void Attacker::perform_single_attack(Creature& attacker, Creature& target, int attackPenalty, const std::string& handName)
{
	// Safety check: ensure target has valid destructible component
	if (!target.destructible)
	{
		game.log("WARNING: Target has no destructible component in perform_single_attack");
		return;
	}

	// if target is shopkeeper, do not attack
	if (dynamic_cast<AiShopkeeper*>(target.ai.get()) && !attacker.has_state(ActorState::IS_RANGED))
	{
		game.menus.push_back(std::make_unique<MenuTrade>(target, attacker));
		return;
	}

	if (!target.destructible->is_dead() && attacker.get_strength() > 0)
	{
		const auto& strength = game.data_manager.get_strength_attributes().at(attacker.get_strength() - 1);

		// Get unified damage using clean interface
		DamageInfo attackDamage = get_attack_damage(attacker);

		// Roll for attack and damage
		int rollAttack = game.d.d20();
		int rollDmg = attackDamage.roll_damage();

		// THAC0 calculation
		int rollNeeded = attacker.destructible->get_thaco() - target.destructible->get_armor_class();

		// Apply dexterity missile attack adjustment if this is a ranged attack
		int hitModifier = attackPenalty; // Start with dual wield penalty
		if (attacker.has_state(ActorState::IS_RANGED))
		{
			const auto& dexAttributes = game.data_manager.get_dexterity_attributes().at(attacker.get_dexterity() - 1);
			hitModifier += dexAttributes.MissileAttackAdj;

			if (dexAttributes.MissileAttackAdj != 0) {
				game.log("Applying ranged attack modifier: " + std::to_string(dexAttributes.MissileAttackAdj) + " from dexterity " + std::to_string(attacker.get_dexterity()));
			}
		}

		// Apply the hit modifier (positive is better, so subtract from roll needed)
		rollNeeded -= hitModifier;

		if (rollAttack >= rollNeeded)
		{
			// Calculate damage
			const int adjDmg = rollDmg + strength.dmgAdj;
			const int totaldmg = adjDmg - target.destructible->get_dr();
			const int finalDamage = std::max(0, totaldmg);

			// Display attack with unified damage info
			game.append_message_part(attacker.actorData.color, attacker.actorData.name);
			game.append_message_part(WHITE_BLACK_PAIR, " (" + handName + ") rolls ");
			game.append_message_part(GREEN_BLACK_PAIR, std::to_string(rollAttack));
			if (attackPenalty != 0)
			{
				game.append_message_part(WHITE_BLACK_PAIR, " (" + std::to_string(attackPenalty) + ")");
			}
			game.append_message_part(WHITE_BLACK_PAIR, " vs " + std::to_string(rollNeeded));
			game.append_message_part(GREEN_BLACK_PAIR, ". Hit! ");
			game.append_message_part(RED_BLACK_PAIR, std::to_string(finalDamage));
			game.append_message_part(WHITE_BLACK_PAIR, " dmg (" + attackDamage.displayRoll + ").");
			game.finalize_message();

			// Enhanced debug logging
			game.log("ATTACK HIT (" + handName + "): " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) +
					 " vs " + std::to_string(rollNeeded) + " needed | Damage: " + std::to_string(rollDmg) +
					 " (" + attackDamage.get_damage_range() + ") + " + std::to_string(strength.dmgAdj) +
					 " str - " + std::to_string(target.destructible->get_dr()) + " DR = " + std::to_string(finalDamage));

			if (finalDamage > 0)
			{
				target.destructible->take_damage(target, finalDamage);
			}
		}
		else
		{
			// Display miss
			game.append_message_part(attacker.actorData.color, attacker.actorData.name);
			game.append_message_part(WHITE_BLACK_PAIR, " (" + handName + ") rolls ");
			game.append_message_part(RED_BLACK_PAIR, std::to_string(rollAttack));
			if (attackPenalty != 0)
			{
				game.append_message_part(WHITE_BLACK_PAIR, " (" + std::to_string(attackPenalty) + ")");
			}
			game.append_message_part(WHITE_BLACK_PAIR, " vs " + std::to_string(rollNeeded));
			game.append_message_part(RED_BLACK_PAIR, ". Miss! ");
			game.append_message_part(RED_BLACK_PAIR, "0");
			game.append_message_part(WHITE_BLACK_PAIR, " dmg.");
			game.finalize_message();

			game.log("ATTACK MISS (" + handName + "): " + attacker.actorData.name + " rolled " + std::to_string(rollAttack) +
					 " vs " + std::to_string(rollNeeded) + " needed");
		}
	}
	else
	{
		game.append_message_part(attacker.actorData.color, std::format("{}", attacker.actorData.name));
		game.append_message_part(WHITE_BLACK_PAIR, std::format(" attacks "));
		game.append_message_part(target.actorData.color, std::format("{}", target.actorData.name));
		game.append_message_part(WHITE_BLACK_PAIR, std::format(" in vain."));
		game.finalize_message();
	}
}

// Clean unified damage interface - determines the correct damage source and calculates enhanced damage
DamageInfo Attacker::get_attack_damage(Creature& attacker) const
{
    // Try to cast to Player to access equipped weapon
    auto* player = dynamic_cast<Player*>(&attacker);
    if (player)
    {
        // Get equipped weapon from right hand
        Item* weapon = player->get_equipped_item(EquipmentSlot::RIGHT_HAND);
        if (weapon && weapon->is_weapon())
        {
            // Use WeaponDamageRegistry with enhancement data for players with weapons
            const ItemEnhancement* enhancement = weapon->is_enhanced() ? &weapon->get_enhancement() : nullptr;
            return WeaponDamageRegistry::get_enhanced_damage_info(weapon->itemClass, enhancement);
        }

        // Player has no weapon equipped - use unarmed damage
        return WeaponDamageRegistry::get_unarmed_damage_info();
    }

    // For monsters and other creatures, use the legacy damageInfo
    return damageInfo;
}

// Legacy compatibility - delegates to the unified interface
// Get enhanced damage info considering weapon enhancements
DamageInfo Attacker::get_enhanced_weapon_damage(Creature& attacker) const
{
    // Try to cast to Player to access equipped weapon
    auto* player = dynamic_cast<Player*>(&attacker);
    if (player)
    {
        // Get equipped weapon from right hand
        Item* weapon = player->get_equipped_item(EquipmentSlot::RIGHT_HAND);
        if (weapon && weapon->is_weapon())
        {
            // Use WeaponDamageRegistry with enhancement data
            const ItemEnhancement* enhancement = weapon->is_enhanced() ? &weapon->get_enhancement() : nullptr;
            return WeaponDamageRegistry::get_enhanced_damage_info(weapon->itemClass, enhancement);
        }

        // Player has no weapon equipped - use unarmed damage
        return WeaponDamageRegistry::get_unarmed_damage_info();
    }

    // For monsters, use the legacy damageInfo (no enhancements for monsters yet)
    return damageInfo;
}

// Helper function to convert legacy roll strings to DamageInfo
DamageInfo Attacker::parse_damage_from_roll_string(const std::string& rollStr) const
{
    // Map common legacy strings to proper DamageInfo
    if (rollStr == "1d4" || rollStr == "D4") return DamageValues::Dagger();
    if (rollStr == "1d6" || rollStr == "D6") return DamageValues::ShortSword();
    if (rollStr == "1d8" || rollStr == "D8") return DamageValues::LongSword();
    if (rollStr == "1d10" || rollStr == "D10") return DamageValues::GreatSword();
    if (rollStr == "1d12" || rollStr == "D12") return {1, 12, "1d12"};
    if (rollStr == "1d20" || rollStr == "D20") return {1, 20, "1d20"};
    if (rollStr == "1d4+1") return DamageValues::WarHammer();
    if (rollStr == "1d6+1") return {2, 7, "1d6+1"};
    if (rollStr == "2d6") return {2, 12, "2d6"};
    if (rollStr == "1d2") return DamageValues::Unarmed();

    // Default fallback for unrecognized strings
    return DamageValues::Unarmed();
}

// end of file: Attacker.cpp
