// file: MenuName.cpp
#include <string>

#include "MenuName.h"
#include "../Game.h"

void MenuName::menu_name_new() noexcept { menuNameWindow = newwin(menuNameHeight, menuNameWidth, menuNameY, menuNameX); }

void MenuName::menu_name_clear() noexcept { wclear(menuNameWindow); }

void MenuName::menu_name_print(int x, int y, const std::string& text) noexcept { mvwprintw(menuNameWindow, y, x, text.c_str()); }

void MenuName::menu_name_refresh() noexcept { wrefresh(menuNameWindow); }

void MenuName::menu_name_delete() noexcept { delwin(menuNameWindow); }

std::string MenuName::menu_name_input()
{
	wgetnstr(menuNameWindow, name, 13);
	// check if name is empty
	if ( strlen(name) != 0)
	{
		return name;
	}
	else
	{
		return "Player";
	}
}

void MenuName::menu_name_store() { playerName = name; }

void MenuName::menu_name_assign() { game.player->actorData.name = playerName; }

void MenuName::menu_name()
{
	menu_name_new();
	curs_set(1); // show cursor
	echo(); // show input

	menu_name_clear();

	menu_name_print(
		0,
		0,
		"Enter your name: "
	);

	menu_name_input();
	menu_name_store();
	menu_name_assign();

	menu_name_refresh();

	curs_set(0); // hide cursor
	noecho(); // hide input

	menu_name_delete();
	
	// Restore the game display after menu
	game.restore_game_display();
}

// end of file: MenuName.cpp
