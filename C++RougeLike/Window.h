#pragma once
#include <iostream>
#include <curses.h>

class Window
{
    //====
    //add a new curses window for printing debug information
    // newwin() creates a new window with the given number of lines,
    //nlines and columns, ncols.
    //The upper left corner of the window is at line begy, column begx.
    //If nlines is zero, it defaults to LINES - begy; ncols to COLS - begx.
    //Create a new full-screen window by calling newwin(0, 0, 0, 0).
    //WINDOW* newwin(int nlines, int ncols, int begy, int begx);
public:
    const char* wintext = "something";
    //the lenght of wintext
    int wintext_len = strlen(wintext) + 2; // why +2 ?
    // have to refresh stdscr before window
    WINDOW* win = newwin(
        3, // int nlines
        wintext_len, // int ncols
        0, // int begy
        0 // int begx
    );

    void border();
    void text();
    void windowrefresh();

};