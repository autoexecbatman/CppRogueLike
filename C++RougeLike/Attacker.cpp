#include <iostream>

#include "main.h"
#include "Window.h"

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
			
			mvprintw(
				29,
				0,
				"%s attacks %s for %g hit points.\n",
				owner->name,
				target->name,
				power - target->destructible->defense
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