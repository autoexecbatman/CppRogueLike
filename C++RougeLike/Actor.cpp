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
	ai(NULL)
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
    if (ai)
    {
        ai->update(this);
    }
}

//bool Actor::moveOrAttack(int x, int y) 
//{
//    if (engine.map->isWall(y, x))
//    {
//        return false;
//    }
//	
//    for (const auto& actor : engine.actors)
//    {
//		if (actor->x == x && actor->y == y)
//		{
//            mvprintw(29, 0, "The %s laughs at your puny efforts to attack him!\n", actor->name);
//			return false;
//		}
//    }
//    this->x = x;
//	this->y = y;
//	return true;
//}
//====