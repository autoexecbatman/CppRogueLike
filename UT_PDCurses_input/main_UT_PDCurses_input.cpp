#include <curses.h>

int main() {
    initscr();  // initialize the curses library
    cbreak();   // disable line buffering


    printw("Enter your name: ");
    char name[50];
    getnstr(name, sizeof(name));  // get user input

    printw("Hello, %s!", name);
    refresh();  // redraw the screen

    getch();    // wait for user input before exiting
    endwin();   // clean up the curses library
    return 0;
}