#include "MenuSell.h"
#include "Game.h"
#include "Actor/Actor.h"

MenuSell::MenuSell(Creature& player) : player(player)
{

}

MenuSell::~MenuSell()
{

}

void MenuSell::draw()
{
}

void MenuSell::on_key(int key)
{
}

void MenuSell::menu()
{
	while(run)
	{
		draw();
		on_key(getch());
	}
}
