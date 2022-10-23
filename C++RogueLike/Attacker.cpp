#include <iostream>

//#include "main.h"
#include "Actor.h"
#include "Engine.h"
#include "Window.h"
#include "Colors.h"

Attacker::Attacker(float power) : power(power) {}

void Attacker::attack(Actor* owner, Actor* target)
{

	if (target->destructible && !target->destructible->is_dead())
	{

		if (power - target->destructible->defense > 0)
		{
			engine.gui->log_message(
				LIGHT_WALL_PAIR, // color
				"%s attacks %s\n for %g hit points.\n", // message
				owner->name,
				target->name,
				power - target->destructible->defense
			);
		}
		else
		{
			mvprintw(
				29, // y
				0, // x
				"%s attacks %s but it has no effect!\n", // format
				owner->name,
				target->name
			);
		}
		
		target->destructible->take_damage(target, power); // kills the player
		
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
			owner->name,
			target->name
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
