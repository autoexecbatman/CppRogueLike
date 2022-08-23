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
	refresh(); // refresh the screen has to be called for the window to show
	wclear(con); // clear the GUI window
	wstandout(con);
	wcolor_set(con, DARK_GROUND_PAIR, 0);
	wbkgdset(con, '.'); // set the background color of the GUI window

	renderBar( // draw the health bar
		1, // int x
		1, // int y
		BAR_WIDTH, // int bar_width
		"HP:", // const char* name
		engine.player->destructible->hp, // float value
		engine.player->destructible->maxHp, // float maxValue
		HPBARFULL_PAIR, // int barColor
		HPBARMISSING_PAIR // int backColor
	);
	box( // draw a border around the GUI window
		con, // WINDOW* win
		0, // int vertChar
		0 // int horChar
	);
	wrefresh(con); // refresh the GUI window has to be called for window to update
}

void Gui::renderBar(
	int x,
	int y,
	int bar_width,
	const char* name, 
	float value, 
	float maxValue, 
	int barColor, 
	int backColor
)
{
	float ratio = value / maxValue; // ratio of the bar's value to its maximum value
	int barWidth = (int)(ratio * bar_width); // the width of the bar in characters
	wattron(con, COLOR_PAIR(barColor)); // set the color of the bar to barColor
	for (int i = 0; i < barWidth; i++) // print the bar
	{
		mvwaddch( // display the full hp
			con,
			y, 
			x + i + strlen(name),
			'+'
		);
	}
	//{
	//	mvwaddch(con, y, x + i, ' ');
	//}
	wattroff(con, COLOR_PAIR(barColor));
	wattron(con, COLOR_PAIR(backColor));
	for (int i = barWidth; i < bar_width; i++)
	{
		mvwaddch( // display the missing hp
			con, 
			y, 
			x + i + strlen(name),
			'-'
		);
	}
	wattroff(con, COLOR_PAIR(backColor));
	mvwprintw( // display the hp tag
		con, 
		1,
		1, 
		name
	);
	mvwprintw(
		con,
		1,
		24,
		"%d/%d",
		(int)value,
		(int)maxValue
		);
}