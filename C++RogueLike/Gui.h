#pragma once
#include <curses.h>
#include <vector>

class Gui
{
public:
	Gui();
	~Gui();
	void render();
	//We want a handy function to write to the log using curses
	void log_message(int log_message_color, const char* log_message_text, ...);
	

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
	std::vector<LogMessage*> log;


	void renderBar(
		int x,
		int y,
		int width,
		const char* name,
		float value, 
		float maxValue, 
		int barColor, 
		int backColor
	);
	void renderMouseLook();
};