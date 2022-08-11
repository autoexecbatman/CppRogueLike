#include <curses.h>
#include "Actor.h"

//====

//the actor constructor
Actor::Actor(int y, int x, int ch, int col) : y(y), x(x), ch(ch), col(col)
{}

//the actor render function with color
void Actor::render() const
{
    attron(COLOR_PAIR(col));
    mvaddch(y, x, ch);
    attroff(COLOR_PAIR(col));
}
//====