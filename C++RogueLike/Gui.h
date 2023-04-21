// file: Gui.h
#ifndef GUI_H
#define GUI_H

// Description:
// The Gui class is responsible for displaying the gui window
// and all of the information that is displayed in the gui window.
//
// Functions:
// The constructor and destructor are empty
// because the gui window is initialized and deleted
// using explicit calls to the gui_init() and gui_delete() functions.
// 
// Initialize the gui window using the PDCurses library
// 
// 
// Objects:
//
//
// Author: @autoexecbatman

#include <curses.h>
#include <vector>
#include <string>

#include "Persistent.h"
#include "LogMessage.h"

class Gui : public Persistent
{
private:
	int guiHp{ 0 };

	WINDOW* guiWin{ nullptr };

	void gui_new(int height, int width, int starty, int startx) noexcept { guiWin = newwin(height, width, starty, startx); }
	void gui_clear() noexcept { wclear(guiWin); }
	void gui_print(int x, int y, const std::string& text) noexcept { mvwprintw(guiWin, y, x, text.c_str()); }
	void gui_refresh() noexcept { wrefresh(guiWin); }
	void gui_delete() noexcept { delwin(guiWin); }
public:
	void gui_init(); // initialize the gui
	void gui_update(); // update the gui
	void gui_render(); // render the gui

	void gui();
	void render();
	//We want a handy function to write to the log using curses
	void log_message(int log_message_color, const char* log_message_text, ...);
	
	void load(TCODZip& zip);
	void save(TCODZip& zip);

protected:
	WINDOW* con{ nullptr }; // the gui window
	WINDOW* sub{ nullptr }; // a subwindow in the gui
	
	//==LOG==
	void print_container(const std::vector<LogMessage*> log_message);
	std::vector<LogMessage*> log = {}; // the message log


	void renderBar(
		int x,
		int y,
		int width,
		const char* name,
		int value, 
		int maxValue, 
		int barColor, 
		int backColor
	);
	void renderMouseLook();
private:

};

#endif // !GUI_H
// end of file: Gui.h
