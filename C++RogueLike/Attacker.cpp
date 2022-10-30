#include <iostream>

//#include "main.h"
#include "Actor.h"
#include "Engine.h"
#include "Window.h"
#include "Colors.h"

Attacker::Attacker(float power) : power(power) {}

void Attacker::attack(Actor& owner, Actor& target)
{

	if (target.destructible && !target.destructible->is_dead())
	{
		// if the target takes damage then a message is displayed
		if (power - target.destructible->defense > 0)
		{
			// damage message
			engine.gui->log_message(
				LIGHT_WALL_PAIR, // color
				"%s attacks %s for %d hit points.\n", // message
				owner.name,
				target.name,
				power - target.destructible->defense
			);
			
			attron(COLOR_PAIR(owner.col));
			mvprintw(0, 0, "%s", owner.name);
			attroff(COLOR_PAIR(owner.col));
			
			int ownerNameLen = strlen(owner.name);
			const char* attacksThe = " attacks the ";
			int attacksTheLen = strlen(attacksThe);
			mvprintw(0, ownerNameLen, attacksThe);
			attron(COLOR_PAIR(target.col));
			mvprintw(0, ownerNameLen + attacksTheLen, "%s", target.name);
			attroff(COLOR_PAIR(target.col));
			int targetNameLen = strlen(target.name);
			mvprintw(0, ownerNameLen + attacksTheLen + targetNameLen, " for %d hit points.\n", power - target.destructible->defense);
		}
		else
		{
			// failed to damage
			mvprintw(
				29, // y
				0, // x
				"%s attacks %s but it has no effect!\n", // format
				owner.name,
				target.name
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
			owner.name,
			target.name
		);
	}
}

void Attacker::load(TCODZip& zip)
{
	power = zip.getFloat();
}

void Attacker::save(TCODZip& zip)
{
	zip.putFloat(power);
}
