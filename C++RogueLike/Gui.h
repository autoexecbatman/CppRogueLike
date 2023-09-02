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
#include <gsl/pointers>

#include "Persistent.h"
#include "LogMessage.h"

constexpr int GUI_HEIGHT{ 7 };

class Gui : public Persistent
{
public:
	~Gui()
	{
		// TODO : these windows are not in use anymore
		delete con;
		delete sub;
	}
private:
	int guiHp{ 0 }, guiHpMax{ 0 }; // a cache of the player's hp and hpMax

	int guiMessageColor{ 0 }; // the color of the message to be displayed on the gui
	std::string guiMessage{}; // the message to be displayed on the gui

	WINDOW* guiWin{ nullptr };

	void gui_new(int height, int width, int starty, int startx) noexcept { guiWin = newwin(height, width, starty, startx); }
	void gui_clear() noexcept { wclear(guiWin); }
	void gui_print(int x, int y, const std::string& text) noexcept { mvwprintw(guiWin, y, x, text.c_str()); }
	void gui_refresh() noexcept { wrefresh(guiWin); }
	void gui_delete() noexcept { delwin(guiWin); }
public:
	void gui_init(); // initialize the gui
	void gui_shutdown(); // shutdown the gui
	void gui_update(); // update the gui
	void gui_render(); // render the gui

	void gui_print_stats(const std::string& playerName, int guiHp, int guiHpMax, int damage, int dr) noexcept;
	void gui_print_attrs(int str, int dex, int con, int inte, int wis, int cha) noexcept;

	void gui();
	void render();
	
	// this function will be redundant soon
	void log_message(int logMessageColor, const char* logMessageText, ...); // We want a handy function to write to the log using curses basically format with color
	
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

protected:
	gsl::owner<WINDOW*> statsWindow{ nullptr };
	gsl::owner<WINDOW*> messageLogWindow{ nullptr };

	gsl::owner<WINDOW*> con{ nullptr }; // the gui window
	gsl::owner<WINDOW*> sub{ nullptr }; // a subwindow in the gui
	
	//==LOG==
	std::vector<std::shared_ptr<LogMessage>> log; // the message log

	void print_container(const std::vector<std::shared_ptr<LogMessage>>& logMessage);

	void renderBar(
		int x,
		int y,
		int width,
		const char* name,
		int value, 
		int maxValue, 
		int barColor, 
		int backColor
	) noexcept;

	void renderMouseLook();
};

#endif // !GUI_H
// end of file: Gui.h
