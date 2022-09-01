#include <vector>
#include "main.h"

bool Pickable::pick(Actor* owner, Actor* wearer)
{
	if (wearer->container && wearer->container->add(owner))
	{
		engine.actors.erase(std::remove(engine.actors.begin(), engine.actors.end(), owner), engine.actors.end());
		return true;
	}
	return false;
}

bool Pickable::use(Actor* owner, Actor* wearer)
{
	if (wearer->container)
	{
		wearer->container->remove(owner);
		delete owner;
		return true;
	}
	return false;
}

Healer::Healer(float amount) : amount(amount)
{
}

bool Healer::use(Actor* owner, Actor* wearer)
{
	if (wearer->destructible)
	{
		float amountHealed = wearer->destructible->heal(amount);
		if (amountHealed>0)
		{
			return Pickable::use(owner, wearer);
		}
	}
	return false;
}
