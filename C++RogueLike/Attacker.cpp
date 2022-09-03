#include <iostream>

#include "main.h"
#include "Window.h"
#include "Colors.h"

Attacker::Attacker(float power) : power(power) {}

void Attacker::attack(Actor* owner, Actor* target)
{
	Window window;
	
	if (target->destructible && !target->destructible->isDead())
	{
		if (power - target->destructible->defense > 0)
		{
			//std::cout <<
			//	"%s attacks %s for %g hit points.\n"
			//	<< std::endl;
			
			//mvprintw(
			//	29,
			//	0,
			//	"%s attacks %s for %g hit points.\n",
			//	owner->name,
			//	target->name,
			//	power - target->destructible->defense
			//);

			engine.gui->log_message(
				LIGHT_WALL_PAIR, // int log_message_color
				"%s attacks %s for %g hit points.\n", // const char* log_message_text
				owner->name, // const char* ...
				target->name,// const char* ...
				power - target->destructible->defense // const char* ...
			);
		}
		else
		{
			//std::cout <<
			//	"%s attacks %s but it has no effect!\n"
			//	<< std::endl;

			mvprintw(
				29,
				0,
				"%s attacks %s but it has no effect!\n",
				owner->name,
				target->name
			);
		}
		target->destructible->takeDamage(target, power); // kills the player
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