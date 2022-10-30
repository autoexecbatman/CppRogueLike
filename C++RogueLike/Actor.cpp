#include <iostream>
#include <curses.h>
#include <math.h>

//#include "main.h"
#include "libtcod.hpp"
class Actor;
#include "Persistent.h"
#include "Destructible.h"
#include "Attacker.h"
#include "Ai.h"
#include "Pickable.h"
#include "Container.h"
#include "Actor.h"
#include "Map.h"
#include "Engine.h"


//====
//the actor constructor initializes the actor's position,name and color
Actor::Actor(
	int y,
	int x,
	int ch,
	const char* name,
	int col
) : 
	posY(y),
	posX(x),
	ch(ch),
	col(col),
	name(name),
	blocks(true),
	fovOnly(true),
	attacker(nullptr),
	destructible(nullptr),
	ai(nullptr),
	container(nullptr),
	pickable(nullptr)
{}

Actor::~Actor()
{
	if (attacker) delete attacker;
	if (destructible) delete destructible;
	if (ai) delete ai;
	if (container) delete container;
	if (pickable) delete pickable;
}

//====
void Actor::load(TCODZip& zip)
{
	posX = zip.getInt();
	posY = zip.getInt();
	ch = zip.getInt();
	col = zip.getInt();
	name = _strdup(zip.getString());
	blocks = zip.getInt();

	bool hasAttacker = zip.getInt();
	bool hasDestructible = zip.getInt();
	bool hasAi = zip.getInt();
	bool hasPickable = zip.getInt();
	bool hasContainer = zip.getInt();

	if (hasAttacker) 
	{
		attacker = new Attacker(0.0f);
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
		container = new Container(0);
		container->load(zip);
	}
}

void Actor::save(TCODZip& zip)
{
	zip.putInt(posX);
	zip.putInt(posY);
	zip.putInt(ch);
	zip.putInt(col);
	zip.putString(name);
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
	container->add(this);
}

//the monster update
void Actor::update()
{	
	if (ai)
	{
		ai->update(this);
	}
}

// a function to get the distance from an actor to a specific tile of the map
double Actor::get_distance(int tileX, int tileY) const
{
	int dx = Actor::posX - tileX;
	int dy = Actor::posY - tileY;
	
	return sqrt(pow(dx, 2) + pow(dy, 2));
}