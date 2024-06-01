// file: Gui.cpp
#include <iostream>
#include <gsl/util>

#include "Gui.h"
#include "LogMessage.h"
//#include "../main.h"
#include "../Colors/Colors.h"
#include "../Game.h"

// guiWin is the main gui window
// ==== ==== ==== ==== ====
constexpr int PANEL_HEIGHT = 7;
constexpr int PANEL_WIDTH = 118;
constexpr int GUI_Y = 22;
constexpr int GUI_X = 0;
// ==== ==== ==== ==== ====

constexpr int BAR_WIDTH = 20;

constexpr int MSG_HEIGHT = PANEL_HEIGHT - 2;
constexpr int MSG_X = BAR_WIDTH + 2;

// statsWindow is the window that displays the player's stats
// ==== ==== ==== ==== ====
constexpr int STATS_HEIGHT = PANEL_HEIGHT;
constexpr int STATS_WIDTH = 20;
constexpr int STATS_Y = 0;
constexpr int STATS_X = 0;
// ==== ==== ==== ==== ====

// messageLogWindow is the window that displays the message log
// ==== ==== ==== ==== ====
constexpr int LOG_HEIGHT = PANEL_HEIGHT - 2;
constexpr int LOG_WIDTH = 56;
constexpr int LOG_Y = 1;
constexpr int LOG_X = 60;
constexpr int LOG_MAX_MESSAGES = 5;
// ==== ==== ==== ==== ====

void Gui::addDisplayMessage(const std::vector<std::pair<int, std::string>>& message)
{
	displayMessages.push_back(message);
}

void Gui::renderMessages() noexcept {
	// Iterate through each message
	for (const auto& message : displayMessages) {
		// Iterate through each part of the message
		for (const auto& part : message) {
			// Extract color and text
			const int color = part.first;
			const std::string& text = part.second;

			// Render logic here using color and text
		}
	}
}

void Gui::gui_init() noexcept
{
	// main gui window
	gui_new(PANEL_HEIGHT, PANEL_WIDTH, GUI_Y, GUI_X);

	// stats window
	statsWindow = derwin(guiWin, STATS_HEIGHT, STATS_WIDTH, STATS_Y, STATS_X);
	// message log window
	messageLogWindow = derwin(guiWin, LOG_HEIGHT, LOG_WIDTH, LOG_Y, LOG_X);

	// set hp and hpMax
	guiHp = game.player->destructible->hp;
	guiHpMax = game.player->destructible->hpMax;

	gui_clear();
	box(guiWin, 0, 0);

	gui_refresh();
}

void Gui::gui_shutdown() noexcept
{
	delwin(statsWindow); // delete the stats window
	delwin(messageLogWindow); // delete the message log window
	gui_delete(); // delete the main gui window
}

void Gui::gui_update()
{
	if (game.player) { guiHp = game.player->destructible->hp; } // update the gui memory
	
	// message to display
	// if an event has occured, store the message in a private variable
	guiMessage = game.messageToDisplay;
	guiMessageColor = game.messageColor;
}

void Gui::gui_render()
{
	gui_clear();
	
	box(guiWin, 0, 0); // border

	// mouse look
	renderMouseLook();

	// print the player's attributes on guiWin
	gui_print_attrs(
		game.player->strength,
		game.player->dexterity,
		game.player->constitution,
		game.player->intelligence,
		game.player->wisdom,
		game.player->charisma
	);

	gui_print_stats(
		game.player->actorData.name,
		guiHp,
		guiHpMax,
		game.player->attacker->dmg,
		game.player->destructible->dr
	);

	gui_print_log();

	gui_refresh();
}

void Gui::gui_print_log()
{
	// messagesToShow is the number of messages to display based on the size of the attackMessagesWhole vector
	const int messagesToShow = std::min(LOG_MAX_MESSAGES, static_cast<int>(game.attackMessagesWhole.size()));

	for (int i = 0; i < messagesToShow; i++)
	{
		const std::vector<std::pair<int, std::string>>& messageParts = game.attackMessagesWhole.at(game.attackMessagesWhole.size() - 1 - i);

		int currentX = 0; // starting position for each message

		for (const auto& part : messageParts)
		{
			wattron(messageLogWindow, COLOR_PAIR(part.first));  // Turn on color
			mvwprintw(messageLogWindow, i, currentX, part.second.c_str());
			wattroff(messageLogWindow, COLOR_PAIR(part.first)); // Turn off color immediately after
			currentX += part.second.size(); // increment the X position by the length of the message part
		}
	}
	wattroff(messageLogWindow, COLOR_PAIR(guiMessageColor)); // Turn off color
}

void Gui::gui_print_stats(std::string_view playerName, int guiHp, int guiHpMax, int damage, int dr) noexcept
{
	// check if the player has no name input then place default name
	if (game.player->actorData.name.empty()) { game.player->actorData.name = "Player"; }

	// print name
	mvwprintw(statsWindow, 2, 1, "Name: %s", playerName.data());
	// print hp
	mvwprintw(guiWin, 3, 1, "HP:%d/%d", guiHp, guiHpMax);
	//// print attack
	//mvwprintw(guiWin, 4, 1, "Attack:%d", damage);

	// print gold
	mvwprintw(guiWin, 4, 1, "Gold: %d", game.player->playerGold);

	// print defense
	mvwprintw(guiWin, 5, 1, "Defense:%d", dr);
}

void Gui::gui_print_attrs(int strength, int dexterity, int constitution, int intelligence, int wisdom, int charisma) noexcept
{
	// print strength
	mvwprintw(guiWin, 1, 1, "Str: %d ", strength);
	// same row print dexterity
	mvwprintw(guiWin, 1, 9, "Dex: %d ", dexterity);
	// print constitution
	mvwprintw(guiWin, 1, 18, "Con: %d ", constitution);
	// print intelligence
	mvwprintw(guiWin, 1, 27, "Int: %d ", intelligence);
	// print wisdom
	mvwprintw(guiWin, 1, 36, "Wis: %d ", wisdom);
	// print charisma
	mvwprintw(guiWin, 1, 45, "Cha: %d ", charisma);
}

void Gui::gui() noexcept
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
	catch (const std::exception& e) {
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
	catch (const std::exception& e) {
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
	catch (const std::exception& e) {
		std::clog << "Failed to clear windows.\n" << std::endl;
		std::cout << e.what() << std::endl;
	}
	std::clog << "clear ok." << std::endl;
	
	try {
		/*refresh();*/ // refresh the screen has to be called for the window to show
		/*wclear(con);*/
	}
	catch (const std::exception& e) {
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
	catch (const std::exception& e) {
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
	catch (const std::exception& e) {
		std::clog << "Failed to draw border around GUI window.\n" << std::endl;
		std::cout << e.what() << std::endl;
	}
	
	// draw the log
	try {
		//int y = 1;
		//for (const auto message : log)
		//{
		//	wclear(sub);
		//	/*wcolor_set(con, message->logMessageColor, 0);*/
		//	wattron(sub, COLOR_PAIR(message->logMessageColor));
		//	mvwprintw(sub, 0, 0, message->logMessageText);
		//	wattroff(sub, COLOR_PAIR(message->logMessageColor));
		//	y++;
		//}
	}
	catch (const std::exception& e) {
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
	catch (const std::exception& e) {
		std::clog << "Failed to refresh windows.\n" << std::endl;
		std::cout << e.what() << std::endl;
	}

	std::clog << "Gui rendering complete." << std::endl;
}

void Gui::log_message(int log_message_col, const char* logMessageText, ...)
{
	// DEBUG log
	//std::clog << "void Gui::log_message() {}" << std::endl;
	
	std::clog << "log_message_col: " << log_message_col << std::endl;
	std::clog << "log_message_text: " << logMessageText << std::endl;

	//// build the text
	//va_list args;
	//char buf[128];
	//va_start(args, logMessageText);
	//vsprintf_s(buf, logMessageText, args);
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
)
{
	std::cout << "void Gui::renderBar() {}" << std::endl;
	const float ratio = gsl::narrow_cast<float>(value) / gsl::narrow_cast<float>(maxValue); // ratio of the bar's value to its maximum value
	const int barWidth = gsl::narrow_cast<int>(ratio * gsl::narrow_cast<float>(bar_width)); // the width of the bar in characters
	const int nameLength = gsl::narrow_cast<int>(strlen(name)); // the length of the name of the bar

	WINDOW* con = newwin(
		0, // int nlines
		0, // int ncols
		22, // int begy
		0 // int begx
	);

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
	wrefresh(con);
	getch();
	delwin(con);
}

void Gui::print_container(const std::vector<std::shared_ptr<LogMessage>>& logMessage)
{
	int i = 0;
	for (const auto& m : logMessage)
	{
		std::cout << i << m->logMessageText << " ";
		i++;
	}
	std::cout << '\n';
}

void Gui::renderMouseLook()
{
	const int mouseY = Mouse_status.y;
	const int mouseX = Mouse_status.x;

	mvprintw(29, 80, "Mouse_status Y: %d, X: %d", mouseY, mouseX); // display the mouse position

	if (!game.map->is_in_fov(Vector2D{ Mouse_status.y, Mouse_status.x }))
	{
		//if the mouse is out of fov, nothing to render
		return;
	}

	std::string buf;
	bool first = true;
	for (const auto& actor : game.creatures)
	{
		if (actor->position.x == mouseX && actor->position.y == mouseY)
		{
			if (!first)
			{
				buf += ", ";
			}
			else
			{
				first = false;
			}
			buf += actor->actorData.name;
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
		zip.putString(message->logMessageText.c_str());
		zip.putInt(message->logMessageColor);
	}
}

void Gui::load(TCODZip& zip)
{
	int nbMessages = zip.getInt();
	while (nbMessages > 0) 
	{
		const char* text = zip.getString();
		const int col = zip.getInt();
		log_message(col, text);
		nbMessages--;
	}
}

// end of file: Gui.cpp
