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

#include <curses.h>
#include <vector>
#include <string>

#include "../Persistent/Persistent.h"
#include "LogMessage.h"

inline constexpr int GUI_HEIGHT{ 7 };
inline constexpr int GUI_WIDTH{ 118 };

class Gui : public Persistent
{
private:
	int guiHp{ 0 }, guiHpMax{ 0 }; // a cache of the player's hp and hpMax
	int guiMessageColor{ 0 }; // the color of the message to be displayed on the gui
	std::string guiMessage{}; // the message to be displayed on the gui
	std::vector<std::vector<LogMessage>> displayMessages; // a container to read messages from, having a color and a string as a pair

	WINDOW* guiWin{ nullptr };

	void gui_new(int height, int width, int starty, int startx) noexcept { guiWin = newwin(height, width, starty, startx); }
	void gui_clear() noexcept { wclear(statsWindow); wclear(messageLogWindow); wclear(guiWin); }
	void gui_print(int x, int y, const std::string& text) noexcept { mvwprintw(guiWin, y, x, text.c_str()); }
	void gui_refresh() noexcept { wrefresh(statsWindow); wrefresh(messageLogWindow); wrefresh(guiWin); }
	void gui_delete() noexcept { delwin(guiWin); }
public:
	bool guiInit{ false };

	void gui_init() noexcept; // initialize the gui
	void gui_shutdown() noexcept; // shutdown the gui
	void gui_update(); // update the gui
	void gui_render(); // render the gui

	void gui_print_stats(std::string_view playerName, int guiHp, int guiHpMax, std::string_view roll, int dr) noexcept;
	void gui_print_log();
	void gui_print_attrs(int str, int dex, int con, int inte, int wis, int cha) noexcept;

	void gui() noexcept;
	
	void load(const json& j) override;
	void save(json& j) override;

	void addDisplayMessage(const std::vector<LogMessage>& message);
	void renderMessages() noexcept;

protected:
	WINDOW* statsWindow{ nullptr };
	WINDOW* messageLogWindow{ nullptr };

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
};

#endif // !GUI_H
// end of file: Gui.h
