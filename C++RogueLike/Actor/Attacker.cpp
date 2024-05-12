// file: Attacker.cpp
#include <iostream>
#include <string>
#include <gsl/gsl>
#include <gsl/util>
#include <format>

#include "../Game.h"
#include "../Colors/Colors.h"
#include "../Actor/Actor.h"
#include "../ActorTypes/LongSword.h"

Attacker::Attacker(int dmg, int minDmg, int maxDmg) noexcept : dmg(dmg), minDmg(minDmg), maxDmg(maxDmg) {}

void Attacker::attack(const Actor& attacker, Actor& target)
{
	if (!target.destructible->is_dead() && attacker.strength > 0) // if target is not dead and attacker has strength
	{
		const size_t str = static_cast<size_t>(attacker.strength - 1); // -1 to access vector index from 0 and strength starts at 1
		std::vector<StrengthAttributes> attrs = loadStrengthAttributes(); // load the strength attributes from file

		if (str < attrs.size()) // if str is in range of vector size
		{
			StrengthAttributes strength = attrs.at(str); // get the strength attributes for the attacker

			// first 
			RandomDice diceAttack; // create a dice object for attack rolls
			const int rollAttack = diceAttack.d20(); // roll 1d20

			RandomDice diceDmg; // create a dice object for damage rolls
			int rollDmg{};
			// roll for damage based on weapon
			if (attacker.weaponEquipped == "Long Sword")
			{
				rollDmg = diceDmg.d8(); // roll 1d8
			}
			else if (attacker.weaponEquipped == "Short Sword")
			{
				rollDmg = diceDmg.d6(); // roll 1d6
			}
			else if (attacker.weaponEquipped == "Dagger")
			{
				rollDmg = diceDmg.d4(); // roll 1d4
			}
			else
			{
				rollDmg = diceDmg.d2(); // roll 1d4
			}

			const int rollNeeded = attacker.destructible->thaco - target.destructible->armorClass; // THAC0 calculation

			if (rollAttack >= rollNeeded) // if the attack roll is greater than or equal to the roll needed
			{
				// calculate the adjusted damage
				const int adjDmg = rollDmg + strength.dmgAdj; // add strength bonus
				const int totaldmg = adjDmg - target.destructible->dr; // substract damage reduction

				// if damage is dealt display combat messages
				if (totaldmg > 0)
				{
					game.appendMessagePart(attacker.col, std::format("{}", attacker.name));
					game.appendMessagePart(WHITE_PAIR, " attacks the ");
					game.appendMessagePart(target.col, std::format("{}", target.name));
					game.appendMessagePart(WHITE_PAIR, std::format(" for {} hit points.", totaldmg));
					game.finalizeMessage();
					// apply damage to target
					target.destructible->take_damage(target, totaldmg);
				}
				// else no damage message
				else
				{
					game.appendMessagePart(attacker.col, std::format("{}", attacker.name));
					game.appendMessagePart(WHITE_PAIR, std::format(" attacks "));
					game.appendMessagePart(target.col, std::format("{}", target.name));
					game.appendMessagePart(WHITE_PAIR, std::format(" but it has no effect!"));
					game.finalizeMessage();
				}
			}
			else
			{
				game.appendMessagePart(attacker.col, std::format("{}", attacker.name));
				game.appendMessagePart(WHITE_PAIR, std::format(" attacks "));
				game.appendMessagePart(target.col, std::format("{}", target.name));
				game.appendMessagePart(WHITE_PAIR, std::format(" and misses."));
				game.finalizeMessage();
			}
		}
		else { game.err("OUT OF BOUNDS!"); return; }
	}
	else
	{
		game.appendMessagePart(attacker.col, std::format("{}", attacker.name));
		game.appendMessagePart(WHITE_PAIR, std::format(" attacks "));
		game.appendMessagePart(target.col, std::format("{}", target.name));
		game.appendMessagePart(WHITE_PAIR, std::format(" in vain."));
		game.finalizeMessage();
	}
}

void Attacker::load(TCODZip& zip)
{
	dmg = zip.getInt();
}

void Attacker::save(TCODZip& zip)
{
	zip.putInt(dmg);
}

// end of file: Attacker.cpp
