#include <span>

#include "MenuBuy.h"
#include "Game.h"

MenuBuy::MenuBuy()
{
	menu_new( menu_height, menu_width, menu_starty, menu_startx );
}

MenuBuy::~MenuBuy()
{
	menu_delete();
}

void MenuBuy::draw()
{
	// print items to buy from shopkeeper
	const auto& inv = game.shopkeeper->container->inv;
	menu_clear();
	for (size_t i{ 0 }; i < inv.size(); ++i)
	{
		mvwprintw(menuWindow, i + 1, 1, inv[i]->actorData.name.c_str());
	}
	menu_refresh();
}

void MenuBuy::on_key(int key)
{
	switch (key)
	{

	}
}

void MenuBuy::menu()
{
	while (run)
	{
		draw();
		menu_key_listen();
		on_key(keyPress);
	}
}
