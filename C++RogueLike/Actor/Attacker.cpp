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
		const int rollAttack = game.d.d20();
		int rollDmg = game.d.roll_from_string(roll);
		// THAC0 calculation
		const int rollNeeded = attacker.destructible->thaco - target.destructible->armorClass;

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
