#ifndef PROJECT_PATH_GUI_H_
#define PROJECT_PATH_GUI_H_


#include <curses.h>
#include <vector>
#include "Persistent.h"

class Gui : public Persistent
{
public:	
	Gui();
	~Gui();
	void render();
	//We want a handy function to write to the log using curses
	void log_message(int log_message_color, const char* log_message_text, ...);
	
	void load(TCODZip& zip);
	void save(TCODZip& zip);
	
	void gui_clear();



protected:
	WINDOW* con; // the gui window
	WINDOW* sub; // a subwindow in the gui
	
	//Create a struct to be able to define the color of each line in the log.
	//So we need a structure to store the message's text and its color.
	//Since this structure is only used by the Gui class, we put it in its protected declaration zone.
	
	struct LogMessage
	{
		char* log_message_text = nullptr;
		int log_message_color = 0;
		
		LogMessage(const char* log_message_text, int log_message_color);
		~LogMessage();
	};
	void print_container(const std::vector<LogMessage*> log_message);
	std::vector<LogMessage*> log; // the message log


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

class Menu 
{
public:
	enum class MenuItemCode : int
	{
		NONE,
		NEW_GAME,
		CONTINUE,
		EXIT
	};
	Menu();
	~Menu();
	
	void menu_clear();
	void addItem(MenuItemCode code, const char* label);

	MenuItemCode pick();

protected:

	struct MenuItem 
	{
		MenuItemCode code;
		const char* label;
	};

	std::vector<MenuItem*> items;
};

#endif // !PROJECT_PATH_GUI_H_