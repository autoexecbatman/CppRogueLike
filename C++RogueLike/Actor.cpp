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