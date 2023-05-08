// file: Gui.cpp
#include <iostream>
#include <gsl/util>

#include "Gui.h"
#include "main.h"
#include "Colors.h"

constexpr int PANEL_HEIGHT = 7;
constexpr int PANEL_WIDTH = 30;
constexpr int GUI_Y = 22;
constexpr int GUI_X = 0;

constexpr int BAR_WIDTH = 20;

constexpr int MSG_HEIGHT = PANEL_HEIGHT - 2;
constexpr int MSG_X = BAR_WIDTH + 2;

void Gui::gui_init()
{
	
	gui_new(PANEL_HEIGHT, PANEL_WIDTH, GUI_Y, GUI_X);

	if (game.player->destructible)
	{
		guiHp = game.player->destructible->hp;
	}

	gui_clear();
	box(guiWin, 0, 0);
	mvwprintw(guiWin, 1, 1, "HP:%d", guiHp);
	gui_refresh();
}

void Gui::gui_shutdown()
{
	gui_delete();
}

void Gui::gui_update()
{
	if (game.player) { guiHp = game.player->destructible->hp; } // update the gui memory
}

void Gui::gui_render()
{
	gui_clear();
	box(guiWin, 0, 0);
	mvwprintw(guiWin, 1, 1, "HP:%d", guiHp);
	renderMouseLook();
	gui_refresh();
}

void Gui::gui()
{
	refresh();

	/*gui_new(PANEL_HEIGHT, PANEL_WIDTH, GUI_Y, GUI_X);*/
	gui_clear();
	box(guiWin, 0, 0);

	// print the text
	mvwprintw(guiWin, 1, 1, "HP:%d", game.player->destructible->hp);

	gui_refresh();
}

void Gui::render()
{
	std::clog << "Gui::render();" << std::endl;
	std::clog << "making con..." << std::endl;
	try {
		//con = newwin(
		//	0,/*PANEL_HEIGHT,*/ // int nlines
		//	0,/*30,*/ // int ncols
		//	22, // int begy
		//	0 // int begx
		//);
	}
	catch (std::exception& e) {
		std::clog << "Failed to initialize con window.\n" << std::endl;
		std::clog << e.what() << std::endl;
	}

	std::clog << "sub = derwin();" << std::endl;
	try {
		//sub = derwin(
		//	con, // 
		//	0,//4, // int nlines
		//	0,//28, // int ncols
		//	0,/*2,*/ // int begy
		//	0/*1*/ // int begx
		//);
	}
	catch (std::exception& e) {
		std::clog << "Failed to initialize sub window.\n" << std::endl;
		std::cout << e.what() << std::endl;
	}
	

	// DEBUG log
	std::clog << "wclear(con);" << std::endl;
	std::clog << "wclear(sub);" << std::endl;
	try {
		/*wclear(con);*/
		/*wclear(sub);*/
	}
	catch (std::exception& e) {
		std::clog << "Failed to clear windows.\n" << std::endl;
		std::cout << e.what() << std::endl;
	}
	std::clog << "clear ok." << std::endl;
	
	try {
		/*refresh();*/ // refresh the screen has to be called for the window to show
		/*wclear(con);*/
	}
	catch (std::exception& e) {
		std::clog << "Failed to refresh screen.\n" << std::endl;
		std::cout << e.what() << std::endl;
	}
	std::clog << "refresh ok." << std::endl;
	try {
		//renderBar( // draw the health bar
		//	1, // int x
		//	1, // int y
		//	BAR_WIDTH, // int bar_width
		//	"HP:", // const char* name
		//	engine.player->destructible->hp, // float value
		//	engine.player->destructible->hpMax, // float maxValue
		//	HPBARFULL_PAIR, // int barColor
		//	HPBARMISSING_PAIR // int backColor
		//);
	}
	catch (std::exception& e) {
		std::clog << "Failed to render health bar.\n" << std::endl;
		std::cout << e.what() << std::endl;
	}

	try {
		//box( // draw a border around the GUI window
		//	con, // WINDOW* win
		//	0, // int vertChar
		//	0 // int horChar
		//);
	}
	catch (std::exception& e) {
		std::clog << "Failed to draw border around GUI window.\n" << std::endl;
		std::cout << e.what() << std::endl;
	}
	
	// draw the log
	try {
		//int y = 1;
		//for (const auto message : log)
		//{
		//	wclear(sub);
		//	/*wcolor_set(con, message->log_message_color, 0);*/
		//	wattron(sub, COLOR_PAIR(message->log_message_color));
		//	mvwprintw(sub, 0, 0, message->log_message_text);
		//	wattroff(sub, COLOR_PAIR(message->log_message_color));
		//	y++;
		//}
	}
	catch (std::exception& e) {
		std::clog << "Failed draw." << std::endl;
		std::cout << e.what() << std::endl;
	}

	std::clog << "renderMouseLook();" << std::endl;
	try {
		// draw the mouse look
		renderMouseLook();
	}
	catch (...) {
		std::clog << "Failed to render mouse look.\n" << std::endl;
	}
	std::clog << "renderMouseLook() ok." << std::endl;

	// draw the XP bar
	//PlayerAi* ai = (PlayerAi*)engine.player->ai;
	//char xpTxt[128];
	//sprintf(xpTxt, "XP(%d)", ai->xpLevel);
	//renderBar(1, 5, BAR_WIDTH, xpTxt, engine.player->destructible->xp,
	//	ai->getNextLevelXp(),
	//	WHITE_PAIR, DRAGON_PAIR);

	try {
/*		wrefresh(con);
		wrefresh(sub);*/// refresh the GUI window has to be called for window to update
	}
	catch (std::exception& e) {
		std::clog << "Failed to refresh windows.\n" << std::endl;
		std::cout << e.what() << std::endl;
	}

	std::clog << "Gui rendering complete." << std::endl;
}

void Gui::log_message(int log_message_col, const char* log_message_text, ...)
{
	// DEBUG log
	//std::clog << "void Gui::log_message() {}" << std::endl;
	
	std::clog << "log_message_col: " << log_message_col << std::endl;
	std::clog << "log_message_text: " << log_message_text << std::endl;

	//// build the text
	//va_list args;
	//char buf[128];
	//va_start(args, log_message_text);
	//vsprintf_s(buf, log_message_text, args);
	//va_end(args);
	//
	//char* lineBegin = buf;
	//char* lineEnd = nullptr;

	//do
	//{
	//	// make room for the new message
	//	//if (log.size() == MSG_HEIGHT)
	//	//{
	//	//	log.erase(log.begin());
	//	//}

	//	// detect end of line
	//	//lineEnd = strchr(lineBegin, '\n');
	//	//if (lineEnd) 
	//	//{
	//	//	*lineEnd = '\0';
	//	//}

	//	// add a new message to the log
	//	LogMessage* msg = new LogMessage(lineBegin, log_message_col);
	//	/*log.push_back(msg);*/

	//	// go to next line
	//	lineBegin = lineEnd + 1;
	//} while (lineEnd);

	//print_container(log);
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
	int value,
	int maxValue,
	int barColor,
	int backColor
) noexcept
{
	std::cout << "void Gui::renderBar() {}" << std::endl;
	const float ratio = gsl::narrow_cast<float>(value) / gsl::narrow_cast<float>(maxValue); // ratio of the bar's value to its maximum value
	const int barWidth = gsl::narrow_cast<int>(ratio * gsl::narrow_cast<float>(bar_width)); // the width of the bar in characters
	const int nameLength = gsl::narrow_cast<int>(strlen(name)); // the length of the name of the bar

	wattron(con, COLOR_PAIR(barColor)); // set the color of the bar to barColor
	for (int i = 0; i < barWidth; i++) // print the bar
	{
		mvwaddch( // display the full hp
			con,
			y, 
			x + i + nameLength,
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
			x + i + nameLength,
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
		gsl::narrow_cast<int>(value),
		gsl::narrow_cast<int>(maxValue)
		);
}

//Gui::LogMessage::LogMessage(const char* log_message_text, int log_message_color) : log_message_color(log_message_color)
//{
//	std::cout << "Gui::LogMessage::LogMessage() {}" << std::endl;
//	this->log_message_text = new char[strlen(log_message_text)]; // allocate memory for the text
//	strcpy(this->log_message_text, log_message_text); // copy the text
//}
//
//Gui::LogMessage::~LogMessage()
//{
//	delete[]log_message_text; // free the memory allocated for the text
//}

void Gui::print_container(const std::vector<std::shared_ptr<LogMessage>>& logMessage)
{
	int i = 0;
	for (const auto& m : logMessage)
	{
		std::cout << i << m->log_message_text << " ";
		i++;
	}
	std::cout << '\n';
}

void Gui::renderMouseLook()
{
	const int mouseX = Mouse_status.x;
	const int mouseY = Mouse_status.y;
	mvprintw(29, 80, "Mouse_status Y: %d, X: %d", mouseY, mouseX); // display the mouse position

	if (!game.map->is_in_fov(Mouse_status.x, Mouse_status.y))
	{
		//if the mouse is out of fov, nothing to render
		return;
	}

	std::string buf;
	bool first = true;
	for (const auto& actor : game.actors)
	{
		if (actor->posX == mouseX && actor->posY == mouseY)
		{
			if (!first)
			{
				buf += ", ";
			}
			else
			{
				first = false;
			}
			buf += actor->name;
		}
	}
	mvwprintw(
		guiWin,
		0,
		1,
		buf.c_str()
	);
}

void Gui::save(TCODZip& zip)
{
	const int logSize = gsl::narrow_cast<int>(log.size());
	zip.putInt(logSize);
	for (const auto& message : log)
	{
		zip.putString(message->log_message_text.c_str());
		zip.putInt(message->log_message_color);
	}
}

void Gui::load(TCODZip& zip)
{
	int nbMessages = zip.getInt();
	while (nbMessages > 0) 
	{
		const char* text = zip.getString();
		int col = zip.getInt();
		log_message(col, text);
		nbMessages--;
	}
}

//==MENU==
//Menu::Menu()
//{
//	// create a new window for the menu using curses
//	WINDOW* menu = newwin(0, 0, 0, 0);
//	// place a box
//	box(menu,0,0);
//}
//
//Menu::~Menu()
//{
//	menu_clear();
//}
//
//void Menu::menu_clear() 
//{
//	items.clear();
//}
//
//void Menu::addItem(MenuItemCode code, const char* label) 
//{
//	MenuItem* item = new MenuItem();
//	item->code = code;
//	item->label = label;
//	items.push_back(item);
//}
//
//Menu::MenuItemCode Menu::pick()
//{
//	/*static TCODImage img("menu_background1.png");*/
//	int selectedItem = 0;
//
//	while (game.run == true)
//	{
//		/*img.blit2x(TCODConsole::root, 0, 0);*/
//		int currentItem = 0;
//		/*for (MenuItem** it = items.begin(); it != items.end(); it++)*/
//		for (MenuItem* item : items)
//		{
//			if (currentItem == selectedItem) 
//			{
//				/*TCODConsole::root->setDefaultForeground(TCODColor::lighterOrange);*/
//			}
//			else 
//			{
//				/*TCODConsole::root->setDefaultForeground(TCODColor::lightGrey);*/
//			}
//			/*TCODConsole::root->print(10, 10 + currentItem * 3, (*it)->label);*/
//			
//			currentItem++;
//		}
//		/*TCODConsole::flush();*/
//
//		//// check key presses
//		//TCOD_key_t key;
//		//TCODSystem::checkForEvent(TCOD_EVENT_KEY_PRESS, &key, NULL);
//		//switch (key.vk) {
//		//case TCODK_UP:
//		//	selectedItem--;
//		//	if (selectedItem < 0) {
//		//		selectedItem = items.size() - 1;
//		//	}
//		//	break;
//		//case TCODK_DOWN:
//		//	selectedItem = (selectedItem + 1) % items.size();
//		//	break;
//		//case TCODK_ENTER:
//		//	return items.get(selectedItem)->code;
//		//default: break;
//		//}
//	}
//	return MenuItemCode::NONE;
//}

// end of file: Gui.cpp
