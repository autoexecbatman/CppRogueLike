#include <iostream>

#include "main.h"

Attacker::Attacker(float power) : power(power) {}

void Attacker::attack(Actor* owner, Actor* target)
{
	if (target->destructible && !target->destructible->isDead())
	{
		if (power - target->destructible->defense > 0)
		{
			std::cout <<
				"%s attacks %s for %g hit points.\n"
				<< std::endl;
		}
		else
		{
			std::cout <<
				"%s attacks %s but it has no effect!\n"
				<< std::endl;
		}
	}
	else
	{
		std::cout << 
			"%s attacks %s in vain.\n"
			<< std::endl;
	}
}