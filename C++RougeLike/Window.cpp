#include "Window.h"


void Window::border()
{
    // draw a border around the edge of the window.
    wborder(
        win, // WINDOW* win
        '-', // ls    left side of border             ACS_VLINE
        '-', // rs    right side of border            ACS_VLINE
        '-', // ts    top side of border              ACS_HLINE
        '-', // bs    bottom side of border           ACS_HLINE
        '-', //tl    top left corner of border       ACS_ULCORNER
        '-', //tr    top right corner of border      ACS_URCORNER
        '-', //bl    bottom left corner of border    ACS_LLCORNER
        '-' //br    bottom right corner of border   ACS_LRCORNER
    );
}

void Window::text()
{
//mvwprintw(
//    win,
//    1,
//    1,
//    wintext
//);
}

void Window::windowrefresh()
{
    wrefresh(win);
}