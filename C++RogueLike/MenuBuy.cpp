#include <span>

#include "MenuBuy.h"
#include "Game.h"

void MenuBuy::populate_items()
{
	const auto& inv = game.shopkeeper->container->inv;
	for (const auto& item : inv)
	{
		menuItems.push_back(item->actorData.name);
	}
}

MenuBuy::MenuBuy()
{
	menu_new( menu_height, menu_width, menu_starty, menu_startx );
	populate_items();
}

MenuBuy::~MenuBuy()
{
	menu_delete();
}

void MenuBuy::menu_print_state(size_t state)
{
	auto row = state;
	if (currentState == state)
	{
		menu_highlight_on();
	}
	menu_print(1, row, menu_get_string(state));
	if (currentState == state)
	{
		menu_highlight_off();
	}
}

void MenuBuy::draw()
{
	// print items to buy from shopkeeper
	const auto& inv = game.shopkeeper->container->inv;
	menu_clear();
	for (size_t i{ 0 }; i < inv.size(); ++i)
	{
		menu_print_state(i);
	}
	menu_refresh();
}

void MenuBuy::on_key(int key)
{
	switch (key)
	{
		case KEY_UP:
			currentState = (currentState + menuItems.size() - 1) % menuItems.size();
			break;
		case KEY_DOWN:
			currentState = (currentState + 1) % menuItems.size();
			break;
		case 27: // ESC
			menu_set_run_false();
			break;
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
