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

	// create window (height, width, starty, startx)
	menuWindow = newwin(height, width, starty, startx);
}