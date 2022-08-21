#include "Window.h"
#include "main.h"

//create a new window function that will create the window when it is called
//it will contain the following lines
//WINDOW* win = newwin(
//3, // int nlines
//wintext_len(wintext), // int ncols
//0, // int begy
//0 // int begx
//);
//auto Window::create_window()
//{
//    WINDOW* win = newwin(
//    3, // int nlines
//    wintext_len(wintext), // int ncols
//    0, // int begy
//    0 // int begx
//    );
//}
//


int wintext_len(const char* wintext)
{
	return strlen(wintext + 2);
}

void Window::create_window(int nlines, int begy, int begx, const char* wintext)
{
    int len = strlen(wintext);
	WINDOW* win = newwin(
		nlines, // int nlines
		len + 2, // int ncols
		begy, // int begy
		begx // int begx
	);
    box(win, 0, 0);
    mvwprintw(
        win,
        1,
        1,
        wintext
    );
    wrefresh(win);
}

void Window::border()
{
    //// draw a border around the edge of the window.
    //wborder(
    //    win, // WINDOW* win
    //    '-', // ls    left side of border             ACS_VLINE
    //    '-', // rs    right side of border            ACS_VLINE
    //    '-', // ts    top side of border              ACS_HLINE
    //    '-', // bs    bottom side of border           ACS_HLINE
    //    '-', //tl    top left corner of border       ACS_ULCORNER
    //    '-', //tr    top right corner of border      ACS_URCORNER
    //    '-', //bl    bottom left corner of border    ACS_LLCORNER
    //    '-' //br    bottom right corner of border   ACS_LRCORNER
    //);

    /*box(win, 0, 0);*/
}

void Window::text(const char* wintext)
{	
    //mvwprintw(
    //    win,
    //    1,
    //    1,
    //    wintext
    //);

	//print text of the players hp in the window win
    //mvprintw(0, 0, "HP: %d", player->destructible->hp); // print the player's hp in the top left corner
}

void Window::windowrefresh()
{
    /*wrefresh(win);*/
}
