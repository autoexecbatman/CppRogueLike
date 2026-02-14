// file: Menu.cpp
#include <string>

#include "Menu.h"
#include "MenuGender.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Systems/GameStateManager.h"
#include "../Systems/MenuManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"
#include "../Gui/Gui.h"

void NewGame::on_selection(GameContext& ctx)
{
	ctx.menus->push_back(std::make_unique<MenuGender>(ctx));
}

void LoadGame::on_selection(GameContext& ctx)
{
	// TODO: clear screen and show loading message (was curses clear/mvprintw/refresh)

	ctx.state_manager->load_all(ctx);

	// TODO: clear loading message after load completes (was curses clear/refresh)
}

void Options::on_selection(GameContext& ctx)
{
	// do nothing
}

void Quit::on_selection(GameContext& ctx)
{
    *ctx.run = false;
    *ctx.shouldSave = false;
    ctx.message_system->log("You quit without saving!");
}

Menu::Menu(bool startup, GameContext& ctx) : isStartupMenu(startup)
{
	int vcols = ctx.renderer ? ctx.renderer->get_viewport_cols() : 60;
	int vrows = ctx.renderer ? ctx.renderer->get_viewport_rows() : 34;
	menu_starty = (vrows - menu_height) / 2;
	menu_startx = (vcols - menu_width) / 2;
	menu_new(menu_height, menu_width, menu_starty, menu_startx, ctx);
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
	// TODO: debug current state display (was curses mvwprintw)

	// Draw menu options
	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(static_cast<MenuState>(i));
	}
}

void Menu::draw()
{
	menu_clear();

	// TODO: fill menu background (was curses wbkgd/COLOR_PAIR)
	// TODO: draw box (was curses box)
	// TODO: draw title "Main Menu" (was curses mvwprintw)

	for (size_t i{ 0 }; i < menuStateStrings.size(); ++i)
	{
		menu_print_state(static_cast<MenuState>(i));
	}
	menu_refresh();
}

void Menu::on_key(int key, GameContext& ctx)
{
	switch (key)
	{

	case 0x103: // KEY_UP
	case 'w':
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + iMenuStates.size() - 1) % iMenuStates.size());
		menu_mark_dirty(); // Mark for redraw
		break;
	}

	case 0x102: // KEY_DOWN
	case 's':
	{
		currentState = static_cast<MenuState>((static_cast<size_t>(currentState) + 1) % iMenuStates.size());
		menu_mark_dirty(); // Mark for redraw
		break;
	}

	case 10:
	{ // if a selection is made
		menu_set_run_false(); // stop running this menu loop
		iMenuStates.at(currentState)->on_selection(ctx); // run the selected option
		break;
	}

	case 27: // Escape key
	{
		menu_set_run_false();
		// Only quit the game if this is the startup menu
		if (isStartupMenu)
		{
			// Use the same quit logic as the Quit menu option
			iMenuStates.at(MenuState::QUIT)->on_selection(ctx);
		}
		break;
	}

	case 'n':
	{
		menu_set_run_false();
		iMenuStates.at(MenuState::NEW_GAME)->on_selection(ctx);
		break;
	}

	case 'l':
	{
		menu_set_run_false();
		iMenuStates.at(MenuState::LOAD_GAME)->on_selection(ctx);
		break;
	}

	case 'o':
	{
		menu_set_run_false();
		iMenuStates.at(MenuState::OPTIONS)->on_selection(ctx);
		break;
	}

	case 'q':
	{
		menu_set_run_false(); // stop running menu loop
		iMenuStates.at(MenuState::QUIT)->on_selection(ctx);
		break; // break out of switch and start closing the game
	}

	default:break;
	}
}

void Menu::menu(GameContext& ctx)
{
	// TODO: clear screen (was curses clear/refresh)
	if (ctx.menu_manager->is_game_initialized() && !isStartupMenu)
	{
		// For in-game menu, show the game world behind it
		ctx.rendering_manager->render(ctx);
		ctx.gui->gui_render(ctx);
	}

	while (run) // menu has its own loop
	{
		draw(); // draw the menu
		menu_key_listen(); // listen for key presses
		on_key(keyPress, ctx); // run the key press
	}

	// Restore full game view if returning to game
	if (ctx.menu_manager->is_game_initialized() && !isStartupMenu)
	{
		// TODO: clear screen (was curses clear/refresh)
		ctx.rendering_manager->render(ctx);
		ctx.gui->gui_render(ctx);
	}
}

// end of file: Menu.cpp
