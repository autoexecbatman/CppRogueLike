// test curses functionality to draw to the screen

#include <curses.h>

int main()
{
	initscr(); // initialize the curses library
	WINDOW* win = newwin(10, 10, 10, 10);
	box(win, 0, 0);
	cbreak(); // disable line buffering
	printw("Hello, world!");
	refresh(); // redraw the screen

	wrefresh(win);

	getch(); // wait for user input before exiting
	endwin(); // clean up the curses library
	return 0;
}