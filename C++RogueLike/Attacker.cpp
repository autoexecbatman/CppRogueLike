// file: Attacker.cpp
#include <iostream>
#include <string>
#include <gsl/gsl>
#include <gsl/util>
#include <format>

#include "Colors.h"
#include "Window.h"
#include "Game.h"
#include "Actor.h"
#include "LongSword.h"

Attacker::Attacker(int dmg, int minDmg, int maxDmg) noexcept : dmg(dmg), minDmg(minDmg), maxDmg(maxDmg) {}

void Attacker::attack(const Actor& attacker, Actor& target)
{
	if (!target.destructible) { game.log("Attacker::attack() - target.destructible is null."); return; }

	if (!target.destructible->is_dead() && attacker.strength > 0) // if target is not dead and attacker has strength
	{
		int str = attacker.strength - 1; // -1 to access vector index from 0
		std::vector<StrengthAttributes> attrs = loadStrengthAttributes();
		try
		{ // try to catch out of range errors
			if (str >= 0 && str < static_cast<int>(attrs.size())) // if str is in range of vector size
			{
				RandomDice diceDmg; // create a dice object
				int rollDmg{};
				// roll for damage based on weapon
				// 1. get the weapon
				if (attacker.weaponEquipped == "long sword")
				{
					rollDmg = diceDmg.d8(); // roll 1d8
					clear();
					// print  "you are using a long sword"
					mvprintw(0, 0, "you are using a long sword");
					refresh();
					getch();
				}
				else
				{
					rollDmg = diceDmg.d4(); // roll 1d4
				}

				RandomDice diceAttack; // create a dice object
				int rollAttack = diceAttack.d20(); // roll 1d20

				StrengthAttributes strength = attrs[str]; // get the strength attributes for the attacker
				//int rollNeeded = attacker.destructible->thaco - (20 - target.destructible->armorClass); // Example THAC0 calculation
				int rollNeeded = 20 - (attacker.destructible->thaco - target.destructible->armorClass); // Example THAC0 calculation
				if (rollAttack >= rollNeeded) // if the attack roll is greater than or equal to the roll needed
				{
					// calculate the adjusted damage
					int adjDmg = rollDmg + strength.dmgAdj; // Adjusted damage
					const int totaldmg = adjDmg - target.destructible->dr;
					dmg = totaldmg;

					//clear();
					//mvprintw(0, 0, "Attacker::attack( %s, %s )", attacker.name.c_str(), target.name.c_str());
					//mvprintw(1, 0, "the current str of the %s is: %d", attacker.name.c_str(), attacker.strength);
					//mvprintw(2, 0, "rollDmg %s : %d", diceDmg.getDiceType().c_str(), rollDmg);
					//mvprintw(3, 0, "strength.dmgAdj: %d", strength.dmgAdj);
					//mvprintw(4, 0, "then calculate damage with bonus from strength(roll+strength.dmgAdj): %d", adjDmg);
					//mvprintw(5, 0, "the target's dr: %d", target.destructible->dr);
					//mvprintw(6, 0, "calculate the totaldmg - reduction: %d", totaldmg);
					//refresh();
					//getch();

					// if damage is dealt display combat messages
					if (totaldmg > 0)
					{
						game.appendMessagePart(attacker.col, std::format("{}", attacker.name));
						game.appendMessagePart(WHITE_PAIR, " attacks the ");
						game.appendMessagePart(target.col, std::format("{}", target.name));
						game.appendMessagePart(WHITE_PAIR, std::format(" for {} hit points.", totaldmg));
						game.finalizeMessage();
						// apply damage to target
						target.destructible->take_damage(target, dmg);
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
		catch (std::out_of_range& e) { game.err(e.what()); return; }
		catch (std::exception& e) { game.err(e.what()); return; }
		catch (...) { game.err("Attacker::attack() - Unknown error."); return; }


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
