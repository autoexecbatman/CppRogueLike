#include <curses.h>
#include "Actor.h"

//====
Actor::Actor(int y, int x, int ch, int col) : y(y), x(x), ch(ch), col(col)
{}

void Actor::render() const
{
    //TCODConsole::root->setChar(x, y, ch);
    //TCODConsole::root->setCharForeground(x, y, col);
    attron(COLOR_PAIR(col));
    mvaddch(y, x, ch);
    attroff(COLOR_PAIR(col));
}
//====