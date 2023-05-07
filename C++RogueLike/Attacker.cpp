// file: Attacker.cpp
#include <iostream>

//#include "main.h"
#include "Actor.h"
#include "Game.h"
#include "Window.h"
#include "Colors.h"

Attacker::Attacker(int power) : power(power) {}

void Attacker::attack(Actor& owner, Actor& target)
{

	if (target.destructible && !target.destructible->is_dead())
	{
		// if the target takes damage then a message is displayed
		if (power - target.destructible->defense > 0)
		{
			// damage message
			game.gui->log_message(
				LIGHTNING_PAIR, // color
				"%s attacks %s for %d hit points.\n", // message
				owner.name.c_str(),
				target.name.c_str(),
				power - target.destructible->defense
			);
			
			attron(COLOR_PAIR(owner.col));
			mvprintw(0, 0, "%s", owner.name.c_str());
			attroff(COLOR_PAIR(owner.col));
			
			size_t ownerNameLen = strlen(owner.name.c_str());
			const char* attacksThe = " attacks the ";
			size_t attacksTheLen = strlen(attacksThe);
			mvprintw(0, ownerNameLen, attacksThe);
			attron(COLOR_PAIR(target.col));
			mvprintw(0, ownerNameLen + attacksTheLen, "%s", target.name.c_str());
			attroff(COLOR_PAIR(target.col));
			size_t targetNameLen = strlen(target.name.c_str());
			mvprintw(0, ownerNameLen + attacksTheLen + targetNameLen, " for %d hit points.\n", power - target.destructible->defense);
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
		target.destructible->take_damage(target, power);
		
	}
	else
	{
		//std::cout << 
		//	"%s attacks %s in vain.\n"
		//	<< std::endl;
		
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
	power = zip.getInt();
}

void Attacker::save(TCODZip& zip)
{
	zip.putInt(power);
}

// end of file: Attacker.cpp
