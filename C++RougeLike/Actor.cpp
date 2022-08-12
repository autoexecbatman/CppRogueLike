#include <curses.h>
#include "Actor.h"
#include "Map.h"
#include "Engine.h"

//====
//the actor constructor initializes the actor's position,name and color
Actor::Actor(int y, int x, int ch, const char* name, int col) : y(y), x(x), ch(ch), col(col) , name(name)
{}

//the actor render function with color
void Actor::render() const
{
    attron(COLOR_PAIR(col));
    mvaddch(y, x, ch);
    attroff(COLOR_PAIR(col));
}
//the monster update
void Actor::update()
{
    printw("the %s growls\n", name);		
}

bool Actor::moveOrAttack(int x, int y) 
{
    if (engine.map->isWall(y, x))
    {
        return false;
    }
	
    for (const auto& actor : engine.actors)
    {
		if (actor->x == x && actor->y == y)
		{
            printw("The %s laughs at your puny efforts to attack him!\n", actor->name);
			return false;
		}
    }
    this->x = x;
	this->y = y;
	return true;
}
//====