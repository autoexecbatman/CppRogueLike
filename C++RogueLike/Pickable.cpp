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

Pickable* Pickable::create(TCODZip& zip) 
{
	PickableType type = (PickableType)zip.getInt();
	Pickable* pickable = nullptr;

	switch (type)
	{
	case PickableType::HEALER:
	{
		pickable = new Healer(0);
		break;
	}
	case PickableType::LIGHTNING_BOLT:
		pickable = new LightningBolt(0, 0);
		break;
	}

	pickable->load(zip);

	return pickable;
}

//==HEALER==
Healer::Healer(int amountToHeal) : amountToHeal(amountToHeal) {}

bool Healer::use(Actor* owner, Actor* wearer)
{
	if (wearer->destructible)
	{
		int amountHealed = wearer->destructible->heal(amountToHeal);

		if (amountHealed > 0)
		{
			return Pickable::use(owner, wearer);
		}
	}

	return false;
}

void Healer::load(TCODZip& zip) 
{
	amountToHeal = zip.getFloat();
}

void Healer::save(TCODZip& zip) 
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::HEALER));
	zip.putFloat(amountToHeal);
}

//==LIGHTNING_BOLT==
LightningBolt::LightningBolt(float maxRange, float damage) : maxRange(maxRange), damage(damage)
{
}

bool LightningBolt::use(Actor* owner, Actor* wearer)
{
	// find closest enemy (inside a maximum range)
	Actor* closestMonster = engine.getClosestMonster(wearer->posX, wearer->posY, maxRange);

	if (!closestMonster)
	{
		engine.gui->log_message(HPBARMISSING_PAIR, "No enemy is close enough to strike.");

		return false;
	}

	// hit closest monster for <damage> hit points
	engine.gui->log_message(HPBARFULL_PAIR, "A lighting bolt strikes the %s with a loud thunder!\n"
		"The damage is %g hit points.", closestMonster->name, damage);
	closestMonster->destructible->takeDamage(closestMonster, damage);

	return Pickable::use(owner, wearer);
}

void LightningBolt::load(TCODZip& zip) 
{
	maxRange = zip.getFloat();
	damage = zip.getFloat();
}

void LightningBolt::save(TCODZip& zip) 
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::LIGHTNING_BOLT));
	zip.putFloat(maxRange);
	zip.putFloat(damage);
}