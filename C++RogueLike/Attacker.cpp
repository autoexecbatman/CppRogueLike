// file: Attacker.cpp
#include <iostream>
#include <gsl/gsl>
#include <gsl/util>

#include "Colors.h"
#include "Window.h"
#include "Game.h"
#include "Actor.h"

Attacker::Attacker(int dmg) noexcept : dmg(dmg) {}

void Attacker::attack(const Actor& owner, Actor& target)
{
	if (target.destructible && !target.destructible->is_dead())
	{
		StrengthAttributes strength = loadStrengthAttributes()[owner.strength - 1];
		int adjDmg = dmg + strength.dmgAdj; // Adjusted damage
		const int totaldmg = adjDmg - target.destructible->dr;

		if (totaldmg > 0)
		{
			game.gui->log_message( // damage message
				LIGHTNING_PAIR, // color
				"%s attacks %s for %d hit points.\n", // message
				owner.name.c_str(),
				target.name.c_str(),
				totaldmg
			);
			
			attron(COLOR_PAIR(owner.col));
			mvprintw(0, 0, "%s", owner.name.c_str());
			attroff(COLOR_PAIR(owner.col));
			const int ownerNameLen = gsl::narrow_cast<int>(owner.name.length());
			std::string attacksThe = " attacks the ";
			const int attacksTheLen = gsl::narrow_cast<int>(attacksThe.length());
			mvprintw(0, ownerNameLen, attacksThe.c_str());
			attron(COLOR_PAIR(target.col));
			mvprintw(0, ownerNameLen + attacksTheLen, "%s", target.name.c_str());
			attroff(COLOR_PAIR(target.col));
			const int targetNameLen = gsl::narrow_cast<int>(target.name.length());
			mvprintw(0, ownerNameLen + attacksTheLen + targetNameLen, " for %d hit points.\n", totaldmg);
		}
		else
		{
			// failed to damage
			mvprintw(
				29, // y
				0, // x
				"%s attacks %s but it has no effect!\n", // format
				owner.name.c_str(),
				target.name.c_str()
			);
		}
		
		// the target takes damage
		target.destructible->take_damage(target, dmg);
		
	}
	else
	{
		mvprintw(
			29,
			0,
			"%s attacks %s in vain.\n",
			owner.name.c_str(),
			target.name.c_str()
		);
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
