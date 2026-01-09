// file: Menu.cpp
#include <string>

#include "Menu.h"
#include "MenuGender.h"
#include "../Game.h"

void NewGame::on_selection()
{
	game.menus.push_back(std::make_unique<MenuGender>());
}

void LoadGame::on_selection()
{
	// Clear the menu from screen before loading
	clear();
	refresh();
	
	// Show loading message
	mvprintw(LINES / 2, (COLS / 2) - 10, "Loading game, please wait...");
	refresh();
	
	game.load_all();
	
	// Clear loading message after load completes
	clear();
	refresh();
}

void Options::on_selection()
{
	// do nothing
}

void Quit::on_selection()
{
	game.run = false;
	game.shouldSave = false;
	game.log("You quit without saving!");
}

Menu::Menu(bool startup) : isStartupMenu(startup)
{
	menu_new(menu_height, menu_width, menu_starty, menu_startx);
	iMenuStates.emplace(MenuState::NEW_GAME, std::make_unique<NewGame>());
	iMenuStates.emplace(MenuState::LOAD_GAME, std::make_unique<LoadGame>());
	iMenuStates.emplace(MenuState::OPTIONS, std::make_unique<Options>());
	iMenuStates.emplace(MenuState::QUIT, std::make_unique<Quit>());
}

Menu::~Menu()
{
	menu_delete();
}

void Menu::menu_print_state(MenuState state)
{
	auto row = static_cast<std::underlying_type_t<MenuState>>(state) + 1; // Start at row 1 after title
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

void Menu::draw_content()
{
	// Debug current state
	mvwprintw(menuWindow, 0, 0, "%d", currentState);
	
	// Draw menu options
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(static_cast<MenuState>(i));
	}
}

void Menu::draw()
{
	menu_clear();
	
	// Fill menu background with solid color to prevent world bleeding
	wbkgd(menuWindow, ' ' | COLOR_PAIR(0)); // Use default color pair
	
	box(menuWindow, 0, 0);
	// Title
	mvwprintw(menuWindow, 0, 1, "Main Menu");
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(static_cast<MenuState>(i));
	}
	menu_refresh();
}

void Menu::on_key(int key)
{
	switch (key)
	{

	case KEY_UP:
	case 'w':
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + iMenuStates.size() - 1) % iMenuStates.size());
		menu_mark_dirty(); // Mark for redraw
		break;
	}

	case KEY_DOWN:
	case 's':
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + 1) % iMenuStates.size());
		menu_mark_dirty(); // Mark for redraw
		break;
	}

	case 10:
	{ // if a selection is made
		menu_set_run_false(); // stop running this menu loop
		iMenuStates.at(currentState)->on_selection(); // run the selected option
		break;
	}

	case 27: // Escape key
	{
		menu_set_run_false();
		// Only quit the game if this is the startup menu
		if (isStartupMenu)
		{
			// Use the same quit logic as the Quit menu option
			iMenuStates.at(MenuState::QUIT)->on_selection();
		}
		break;
	}

	case 'n':
	{
		menu_set_run_false();
		iMenuStates.at(MenuState::NEW_GAME)->on_selection();
		break;
	}

	case 'l':
	{
		menu_set_run_false();
		iMenuStates.at(MenuState::LOAD_GAME)->on_selection();
		break;
	}

	case 'o':
	{
		menu_set_run_false();
		iMenuStates.at(MenuState::OPTIONS)->on_selection();
		break;
	}

	case 'q':
	{
		menu_set_run_false(); // stop running menu loop
		iMenuStates.at(MenuState::QUIT)->on_selection();
		break; // break out of switch and start closing the game
	}

	default:break;
	}
}

void Menu::menu()
{
	// Clear screen and render game background before showing menu
	clear();
	if (game.menu_manager.is_game_initialized() && !isStartupMenu)
	{
		// For in-game menu, show the game world behind it
		auto ctx = game.get_context();
		game.render();
		game.gui.gui_render(ctx);
	}
	refresh();
	
	while (run) // menu has its own loop
	{
		draw(); // draw the menu
		menu_key_listen(); // listen for key presses
		on_key(keyPress); // run the key press
	}
	
	// Restore full game view if returning to game
	if (game.menu_manager.is_game_initialized() && !isStartupMenu)
	{
		clear();
		auto ctx = game.get_context();
		game.render();
		game.gui.gui_render(ctx);
		refresh();
	}
}

// end of file: Menu.cpp
