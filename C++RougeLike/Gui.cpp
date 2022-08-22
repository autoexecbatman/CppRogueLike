#include "Gui.h"
#include "main.h"
#include "Colors.h"


constexpr int PANEL_HEIGHT = 7;
constexpr int BAR_WIDTH = 20;

Gui::Gui()
{
	con = newwin(
		PANEL_HEIGHT, // int nlines
		engine.screenWidth, // int ncols
		22, // int begy
		0 // int begx
	);

}

Gui::~Gui()
{
	delwin(con);
}

void Gui::render()
{
	refresh();
	box(
		con, // WINDOW* win
		0, // int vertChar
		0 // int horChar
	);
	wrefresh(con);
	wbkgdset(con, '.');
	wclear(con);

	renderBar(
		1, // int x
		1, // int y
		BAR_WIDTH, // int width
		"HP", // const char* name
		engine.player->destructible->hp, // float value
		engine.player->destructible->maxHp, // float maxValue
		GUI_PAIR, // int barColor
		GUIBKGD_PAIR // int backColor
	);
}

void Gui::renderBar(int x, int y, int width, const char* name, float value, float maxValue, int barColor, int backColor)
{
	float ratio = value / maxValue; // ratio of the bar's value to its maximum value
	int barWidth = (int)(ratio * width); // the width of the bar in characters
	wattron(con, COLOR_PAIR(barColor)); // set the color of the bar to barColor
	for (int i = 0; i < barWidth; i++) // print the bar
	{
		mvwaddch(con, y, x + i, '=');
	}
	//{
	//	mvwaddch(con, y, x + i, ' ');
	//}
	wattroff(con, COLOR_PAIR(barColor));
	wattron(con, COLOR_PAIR(backColor));
	for (int i = barWidth; i < width; i++)
	{
		mvwaddch(con, y, x + i, ' ');
	}
	wattroff(con, COLOR_PAIR(backColor));
	mvwprintw(con, y, x + width / 2 - strlen(name) / 2, name);
}