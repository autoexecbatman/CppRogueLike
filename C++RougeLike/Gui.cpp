#include <iostream>
#include "Gui.h"
#include "main.h"
#include "Colors.h"


constexpr int PANEL_HEIGHT = 7;
constexpr int BAR_WIDTH = 20;
constexpr int MSG_HEIGHT = PANEL_HEIGHT - 2;
constexpr int MSG_X = BAR_WIDTH + 2;

Gui::Gui()
{
	std::cout << "Gui();" << std::endl;
	std::cout << "con = newwin();" << std::endl;
	con = newwin(
		PANEL_HEIGHT, // int nlines
		engine.screenWidth, // int ncols
		22, // int begy
		0 // int begx
	);
	
	std::cout << "sub = derwin();" << std::endl;
	sub = derwin(
		con, // 
		2, // int nlines
		29, // int ncols
		2, // int begy
		1 // int begx
	);

}

Gui::~Gui()
{
	std::cout << "~Gui();" << std::endl;
	delwin(con);
	/*log.clear();*/
}

void Gui::render()
{
	std::cout << "void Gui::render() {}" << std::endl;
	//wbkgdset(con, '.'); // set the background color of the GUI window
	refresh(); // refresh the screen has to be called for the window to show
	/*wclear(con);*/
	/*wclear(sub);*/
	/*wclear(con);*/ // clear the GUI window
	/*wstandout(con);*/
	/*wcolor_set(con, DARK_GROUND_PAIR, 0);*/
	renderBar( // draw the health bar
		1, // int x
		1, // int y
		BAR_WIDTH, // int bar_width
		"HP:", // const char* name
		engine.player->destructible->hp, // float value
		engine.player->destructible->maxHp, // float maxValue
		HPBARFULL_PAIR, // int barColor
		HPBARMISSING_PAIR // int backColor
	);
	box( // draw a border around the GUI window
		con, // WINDOW* win
		0, // int vertChar
		0 // int horChar
	);
	
	// draw the log
	/*message(LIGHT_GROUND_PAIR, "message log");*/
	for (auto const& message : log)
	{
		wcolor_set(con, message->log_message_color, 0);
		mvwprintw(con, 2, 1, message->log_message_text);
	}

	wrefresh(con); // refresh the GUI window has to be called for window to update

}

void Gui::log_message(int log_message_col, const char* log_message_text, ...)
{
	std::cout << "void Gui::log_message() {}" << std::endl;
	// build the text
	va_list args;
	char buf[128];
	va_start(args, log_message_text);
	vsprintf(buf, log_message_text, args);
	va_end(args);
	
	char* lineBegin = buf;
	char* lineEnd = nullptr;
	do
	{
		// make room for the new message
		//if (log.size() == MSG_HEIGHT)
		//{
		//	log.erase(log.begin());
		//}
		// detect end of line
		lineEnd = strchr(lineBegin, '\n');
		if (lineEnd) 
		{
			*lineEnd = '\0';
		}
		// add a new message to the log
		LogMessage* msg = new LogMessage(lineBegin, 2);
		log.push_back(msg);
		// go to next line
		lineBegin = lineEnd + 1;
	} while (lineEnd);
	print_container(log);
}
//
//	//mvwprintw(con, MSG_HEIGHT, 1, text);
//
//	// build the text
//	va_list ap; // a variable argument list
//	char buf[128]; // a buffer to store the formatted text in
//	va_start(ap, text); // initialize the variable argument list
//	wattron(sub, COLOR_PAIR(message_col)); // set the color of the text
//	vwprintw(sub, text, ap);
//	wattroff(sub, COLOR_PAIR(message_col)); // reset the color of the text
//	va_end(ap);
//	wrefresh(sub);
//
//	//vsprintf(buf, text, ap); // print the text
//	/*wrefresh(con);*/ // refresh the GUI window has to be called for window to update
//
//	char* lineBegin = buf; 
//	char* lineEnd;
//	
//	do 
//	{
//		// make room for the new message
//		if (log.size() == MSG_HEIGHT) 
//		{
//			//Message* toRemove = log.get(0);
//			//log.remove(toRemove);
//			//delete toRemove;
//			log.erase(log.begin());
//		}
//
//		// detect end of the line
//		lineEnd = strchr(lineBegin, '\n');
//		if (lineEnd) 
//		{
//			*lineEnd = '\0';
//		}
//
//		// add a new message to the log
//		//Message* msg = new Message(lineBegin, col);
//		//log.push(msg);
//		log.push_back(new Message(lineBegin, message_col));
//		// go to next line
//		lineBegin = lineEnd + 1;
//	}
//	while (lineEnd);
//}

void Gui::renderBar(
	int x,
	int y,
	int bar_width,
	const char* name, 
	float value, 
	float maxValue, 
	int barColor, 
	int backColor
)
{
	std::cout << "void Gui::renderBar() {}" << std::endl;
	float ratio = value / maxValue; // ratio of the bar's value to its maximum value
	int barWidth = (int)(ratio * bar_width); // the width of the bar in characters
	wattron(con, COLOR_PAIR(barColor)); // set the color of the bar to barColor
	for (int i = 0; i < barWidth; i++) // print the bar
	{
		mvwaddch( // display the full hp
			con,
			y, 
			x + i + strlen(name),
			'+'
		);
	}
	//{
	//	mvwaddch(con, y, x + i, ' ');
	//}
	wattroff(con, COLOR_PAIR(barColor));
	wattron(con, COLOR_PAIR(backColor));
	for (int i = barWidth; i < bar_width; i++)
	{
		mvwaddch( // display the missing hp
			con, 
			y, 
			x + i + strlen(name),
			'-'
		);
	}
	wattroff(con, COLOR_PAIR(backColor));
	mvwprintw( // display the hp tag
		con, 
		1,
		1, 
		name
	);
	mvwprintw(
		con,
		1,
		24,
		"%d/%d",
		(int)value,
		(int)maxValue
		);
}

Gui::LogMessage::LogMessage(const char* log_message_text, int log_message_color) : log_message_color(log_message_color)
{
	std::cout << "Gui::LogMessage::LogMessage() {}" << std::endl;
	this->log_message_text = new char[strlen(log_message_text)]; // allocate memory for the text
	strcpy(this->log_message_text, log_message_text); // copy the text
	
}

Gui::LogMessage::~LogMessage()
{
	delete[]log_message_text; // free the memory allocated for the text
}

void Gui::print_container(std::vector<LogMessage*> message)
{
	int i = 0;
	for (const auto& message : message)
	{
		std::cout << message->log_message_text << i << " ";
		i++;
	}
	std::cout << '\n';
}

//void Engine::print_container(const std::deque<Actor*> actors)
//{
//	int i = 0;
//	for (const auto& actor : actors)
//	{
//		std::cout << actor->name << i << " ";
//		i++;
//	}
//	std::cout << '\n';
//}