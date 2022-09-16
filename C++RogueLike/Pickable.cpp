#include <vector>
#include "main.h"
#include "Colors.h"

//==PICKABLE==
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

Pickable* Pickable::create(TCODZip& zip) {
	PickableType type = (PickableType)zip.getInt();
	Pickable* pickable = NULL;
	switch (type) {
	case HEALER: pickable = new Healer(0); break;
	case LIGHTNING_BOLT: pickable = new LightningBolt(0, 0); break;
	}
	pickable->load(zip);
	return pickable;
}

//==HEALER==
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

void Healer::load(TCODZip& zip) 
{
	amount = zip.getFloat();
}

void Healer::save(TCODZip& zip) 
{
	zip.putInt(HEALER);
	zip.putFloat(amount);
}

//==LIGHTNING_BOLT==
LightningBolt::LightningBolt(float range, float damage) : range(range), damage(damage)
{
}

bool LightningBolt::use(Actor* owner, Actor* wearer)
{
	Actor* closestMonster = engine.getClosestMonster(wearer->x, wearer->y, range);
	if (!closestMonster)
	{
		engine.gui->log_message(HPBARMISSING_PAIR, "No enemy is close enough to strike.");
		return false;
	}
	// hit closest monster for <damage> hit points
	engine.gui->log_message(HPBARFULL_PAIR, "A lighting bolt strikes the % s with a loud thunder!\n"
		"The damage is %g hit points.", closestMonster->name, damage);
	closestMonster->destructible->takeDamage(closestMonster, damage);

	return Pickable::use(owner, wearer);
}

void LightningBolt::load(TCODZip& zip) 
{
	range = zip.getFloat();
	damage = zip.getFloat();
}

void LightningBolt::save(TCODZip& zip) 
{
	zip.putInt(LIGHTNING_BOLT);
	zip.putFloat(range);
	zip.putFloat(damage);
}