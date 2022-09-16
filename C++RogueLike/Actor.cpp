#include <iostream>
#include <curses.h>

#include "main.h"


//====
//the actor constructor initializes the actor's position,name and color
Actor::Actor(
	int y,
	int x,
	int ch,
	const char* name,
	int col
) : 
	y(y),
	x(x),
	ch(ch),
	col(col),
	name(name),
	blocks(true),
	attacker(NULL),
	destructible(NULL),
	ai(NULL),
	container(NULL),
	pickable(NULL)
{}

Actor::~Actor()
{
	if (attacker) delete attacker;
	if (destructible) delete destructible;
	if (ai) delete ai;
	if (pickable) delete pickable;
	if (container) delete container;
}

void Actor::load(TCODZip& zip)
{
	x = zip.getInt();
	y = zip.getInt();
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
	zip.putInt(x);
	zip.putInt(y);
	zip.putInt(ch);
	zip.putInt(col);
	zip.putString(name);
	zip.putInt(blocks);

	zip.putInt(attacker != NULL);
	zip.putInt(destructible != NULL);
	zip.putInt(ai != NULL);
	zip.putInt(pickable != NULL);
	zip.putInt(container != NULL);

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
	mvaddch(y, x, ch);
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
double Actor::getDistance(int cx, int cy) const
{
	int dx = Actor::x - cx;
	int dy = Actor::y - cy;
	
	return sqrt(dx * dx + dy * dy);
}

//====