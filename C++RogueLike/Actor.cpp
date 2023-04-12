#include <iostream>
#include <curses.h>
#include <math.h>
#include <memory>

//#include "main.h"
//#include "libtcod.hpp"
#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)
class Actor;
#include "Persistent.h"
#include "Destructible.h"
#include "Attacker.h"
#include "Ai.h"
#include "Pickable.h"
#include "Container.h"
#include "Actor.h"
#include "Map.h"
#include "Game.h"


//====
//the actor constructor initializes the actor's position,name and color
Actor::Actor(
	int y,
	int x,
	int ch,
	std::string name,
	int col,
	int index
) : 
	posY(y),
	posX(x),
	ch(ch),
	col(col),
	index(index),
	name(name),
	blocks(true),
	fovOnly(true),
	attacker(nullptr),
	destructible(nullptr),
	ai(nullptr),
	container(nullptr),
	pickable(nullptr)
{}

Actor::~Actor() {}

//====
void Actor::load(TCODZip& zip)
{
	posX = zip.getInt();
	posY = zip.getInt();
	ch = zip.getInt();
	col = zip.getInt();
	name = _strdup(zip.getString());
	blocks = zip.getInt();

	const bool hasAttacker = zip.getInt();
	const bool hasDestructible = zip.getInt();
	const bool hasAi = zip.getInt();
	const bool hasPickable = zip.getInt();
	const bool hasContainer = zip.getInt();

	if (hasAttacker) 
	{
		attacker = std::make_shared<Attacker>(0);
		attacker->load(zip);
	}
	if (hasDestructible) 
	{
		destructible = Destructible::create(zip);
	}
	if (hasAi) 
	{
		ai = Ai::create(zip);
	}
	if (hasPickable) 
	{
		pickable = Pickable::create(zip);
	}
	if (hasContainer) 
	{
		container = std::make_shared<Container>(0);
		container->load(zip);
	}
}

void Actor::save(TCODZip& zip)
{
	zip.putInt(posX);
	zip.putInt(posY);
	zip.putInt(ch);
	zip.putInt(col);
	zip.putString(name.c_str());
	zip.putInt(blocks);

	zip.putInt(attacker != nullptr);
	zip.putInt(destructible != nullptr);
	zip.putInt(ai != nullptr);
	zip.putInt(pickable != nullptr);
	zip.putInt(container != nullptr);

	if (attacker) attacker->save(zip);
	if (destructible) destructible->save(zip);
	if (ai) ai->save(zip);
	if (pickable) pickable->save(zip);
	if (container) container->save(zip);
}

//the actor render function with color
void Actor::render() const
{
	attron(COLOR_PAIR(col));
	mvaddch(posY, posX, ch);
	attroff(COLOR_PAIR(col));
}

void Actor::pickItem(int x, int y)
{
	// add item to inventory
	container->add(*this);
	/*container->add(std::shared_ptr<Actor>(this));*/
}

//the monster update
void Actor::update()
{
	// if the actor has an ai then update the ai
	if (ai)
	{
		/*ai->update(this);*/
		/*ai->update(std::shared_ptr<Actor>(this));*/
		/*ai->update(std::shared_ptr<Actor>(this, [](Actor*) {}));*/
		/*ai->update(shared_from_this());*/
		ai->update(*this);
	}
}

// a function to get the distance from an actor to a specific tile of the map
int Actor::get_distance(int tileX, int tileY) const
{
	// using chebyshev distance
	int distance = std::max(abs(posX - tileX), abs(posY - tileY));

	mvprintw(10, 0, "Distance: %d", distance);

	return distance;
}