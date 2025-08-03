#include "BaseMenu.h"
#include "../Game.h"

void BaseMenu::menu_new(int height, int width, int starty, int startx)
{
	// check bound before creating window
	if (height >= LINES || width >= COLS)
	{
		game.log("Menu window size is too big. Height: " + std::to_string(height) + ", Width: " + std::to_string(width));
		game.log("Terminal size - LINES: " + std::to_string(LINES) + ", COLS: " + std::to_string(COLS));
		std::exit(EXIT_FAILURE);
	}

	if (starty < 0 || startx < 0 || starty >= 29 || startx >= 119)
	{
		game.log("Menu window start position is out of bounds. StartY: " + std::to_string(starty) + ", StartX: " + std::to_string(startx));
		std::exit(EXIT_FAILURE);
	}

	// Store dimensions
	menu_height = height;
	menu_width = width;
	menu_starty = starty;
	menu_startx = startx;

	// create main menu window
	menuWindow = newwin(height, width, starty, startx);

	// create background preservation window (slightly larger to include borders)
	backgroundWindow = newwin(height + 2, width + 2, 
							  starty > 0 ? starty - 1 : 0, 
							  startx > 0 ? startx - 1 : 0);

	// Save background before showing menu
	menu_save_background();
	needsRedraw = true;
}

void BaseMenu::menu_delete()
{
	// Restore background before deletion
	menu_restore_background();
	
	if (menuWindow)
	{
		delwin(menuWindow);
		menuWindow = nullptr;
	}
	
	if (backgroundWindow)
	{
		delwin(backgroundWindow);
		backgroundWindow = nullptr;
	}
}

void BaseMenu::menu_save_background()
{
	if (!backgroundWindow) return;
	
	// Copy the screen area that will be covered by the menu
	copywin(stdscr, backgroundWindow, 
			menu_starty > 0 ? menu_starty - 1 : 0,  // source start y
			menu_startx > 0 ? menu_startx - 1 : 0,  // source start x
			0, 0,  // dest start y, x
			menu_height + 1,  // dest end y
			menu_width + 1,   // dest end x
			FALSE);  // don't overlay
}

void BaseMenu::menu_restore_background()
{
	if (!backgroundWindow) return;
	
	// Restore the saved background
	copywin(backgroundWindow, stdscr,
			0, 0,  // source start y, x
			menu_starty > 0 ? menu_starty - 1 : 0,  // dest start y
			menu_startx > 0 ? menu_startx - 1 : 0,  // dest start x
			menu_height + 1,  // dest end y
			menu_width + 1,   // dest end x
			FALSE);  // don't overlay
	
	// Refresh the restored area
	refresh();
}

void BaseMenu::menu_draw_box()
{
	if (!menuWindow) return;
	
	// Clear the window first
	wclear(menuWindow);
	// Draw border box
	box(menuWindow, 0, 0);
}

void BaseMenu::menu_efficient_refresh()
{
	if (!needsRedraw) return;
	
	// Only refresh menu window, not entire screen
	wnoutrefresh(menuWindow);
	doupdate(); // Single screen update for all windows
	
	needsRedraw = false;
}