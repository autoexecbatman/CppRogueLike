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

Attacker::Attacker(int dmg) noexcept : dmg(dmg) {}

void Attacker::attack(const Actor& attacker, Actor& target)
{
	if (!target.destructible) { game.log("Attacker::attack() - target.destructible is null."); return; }

	if (!target.destructible->is_dead() && attacker.strength > 0) // if target is not dead and attacker has strength
	{
		int str = attacker.strength - 1; // -1 to access vector index from 0
		std::vector<StrengthAttributes> attrs = loadStrengthAttributes();
		try
		{
			if (str >= 0 && str < static_cast<int>(attrs.size())) // if str is in range of vector size
			{
				StrengthAttributes strength = attrs[str];
				int adjDmg = dmg + strength.dmgAdj; // Adjusted damage
				const int totaldmg = adjDmg - target.destructible->dr;

				// if damage is dealt display combat messages
				if (totaldmg > 0)
				{
					game.appendMessagePart(attacker.col, std::format("{}", attacker.name));
					game.appendMessagePart(WHITE_PAIR, " attacks the ");
					game.appendMessagePart(target.col, std::format("{}", target.name));
					game.appendMessagePart(WHITE_PAIR, std::format(" for {} hit points.", totaldmg));
					game.finalizeMessage();
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
			else { game.err("OUT OF BOUNDS!"); return; }
		}
		catch (std::out_of_range& e) { game.err(e.what()); return; }
		catch (std::exception& e) { game.err(e.what()); return; }
		catch (...) { game.err("Attacker::attack() - Unknown error."); return; }

		// apply damage to target
		target.destructible->take_damage(target, dmg);

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
