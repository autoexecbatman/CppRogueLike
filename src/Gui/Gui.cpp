// file: Gui.cpp
#include <iostream>
#include <span>

#include "Gui.h"
#include "LogMessage.h"
//#include "../main.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"
#include "../ActorTypes/Player.h"
#include "../Map/Map.h"
#include "../Systems/CreatureManager.h"
#include "../Systems/HungerSystem.h"
#include "../Utils/Vector2D.h"

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

void Gui::add_display_message(const std::vector<LogMessage>& message)
{
	displayMessages.push_back(message);
}

void Gui::render_messages() noexcept {
	// Iterate through each message
	for (const auto& message : displayMessages) {
		// Iterate through each part of the message
		for (const auto& part : message) {
			// Extract color and text
			const int color = part.logMessageColor;
			const std::string& text = part.logMessageText;

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

void Gui::gui_update(GameContext& ctx)
{
	// message to display
	// if an event has occured, store the message in a private variable
	set_message(ctx.message_system->get_current_message());
	set_message_color(ctx.message_system->get_current_message_color());
}

void Gui::gui_render(const GameContext& ctx)
{
	gui_clear();
	box(guiWin, 0, 0); // border

	// mouse look
	renderMouseLook(ctx);

	// print the player's attributes on guiWin
	gui_print_attrs(
		ctx.player->get_strength(),
		ctx.player->get_dexterity(),
		ctx.player->get_constitution(),
		ctx.player->get_intelligence(),
		ctx.player->get_wisdom(),
		ctx.player->get_charisma()
	);

	gui_print_stats(
		ctx,
		ctx.player->actorData.name,
		ctx.player->destructible->get_hp(),
		ctx.player->destructible->get_max_hp(),
		ctx.player->attacker->get_attack_damage(*ctx.player).displayRoll,
		ctx.player->destructible->get_dr()
	);

	render_hp_bar(ctx);
	render_hunger_status(ctx);
	render_player_status(ctx);

	gui_print_log(ctx);

	gui_refresh();
}

void Gui::render_player_status(const GameContext& ctx)
{
	// Render player status effects
	int statusY = 6; // Position below gold display

	if (ctx.player->has_state(ActorState::IS_CONFUSED))
	{
		wattron(guiWin, COLOR_PAIR(WHITE_GREEN_PAIR));
		mvwprintw(guiWin, statusY, 1, "Status: Confused!");
		wattroff(guiWin, COLOR_PAIR(WHITE_GREEN_PAIR));
	}
}

void Gui::gui_print_log(const GameContext& ctx)
{
	// messagesToShow is the number of messages to display based on the size of the attackMessagesWhole vector
	const int messagesToShow = std::min(LOG_MAX_MESSAGES, static_cast<int>(ctx.message_system->get_stored_message_count()));

	for (int i = 0; i < messagesToShow; i++)
	{
		const std::vector<LogMessage>& messageParts = ctx.message_system->get_attack_message_at(ctx.message_system->get_stored_message_count() - 1 - i);

		size_t currentX = 0; // starting position for each message

		for (const auto& part : messageParts)
		{
			wattron(messageLogWindow, COLOR_PAIR(part.logMessageColor));  // Turn on color
			mvwprintw(messageLogWindow, i, static_cast<int>(currentX), part.logMessageText.c_str());
			wattroff(messageLogWindow, COLOR_PAIR(part.logMessageColor)); // Turn off color immediately after
			currentX += part.logMessageText.size(); // increment the X position by the length of the message part
		}
	}
	wattroff(messageLogWindow, COLOR_PAIR(guiMessageColor)); // Turn off color
}

void Gui::gui_print_stats(const GameContext& ctx, std::string_view playerName, int guiHp, int guiHpMax, std::string_view roll, int dr) noexcept
{
	// check if the player has no name input then place default name
	if (ctx.player->actorData.name.empty()) { ctx.player->actorData.name = "Player"; }

	// Print everything to main window to avoid sub-window issues
	mvwprintw(guiWin, 2, 1, "Name: %s", playerName.data());
	mvwprintw(guiWin, 3, 1, "HP:%d/%d", guiHp, guiHpMax);
	mvwprintw(guiWin, 4, 1, "Gold: %d", ctx.player->get_gold());
	mvwprintw(guiWin, 5, 1, "Turn: %d", *ctx.time);
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

void Gui::gui(GameContext& ctx) noexcept
{
	refresh();

	gui_clear();
	box(guiWin, 0, 0);

	// print the text
	mvwprintw(guiWin, 1, 1, "HP:%d", ctx.player->destructible->get_hp());

	gui_refresh();
}

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
	const float ratio = static_cast<float>(value) / static_cast<float>(maxValue); // ratio of the bar's value to its maximum value
	const int barWidth = static_cast<int>(ratio * static_cast<float>(bar_width)); // the width of the bar in characters
	const int nameLength = static_cast<int>(strlen(name)); // the length of the name of the bar

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
		static_cast<int>(value),
		static_cast<int>(maxValue)
		);
	wrefresh(con);
	getch();
	delwin(con);
}

void Gui::renderMouseLook(const GameContext& ctx)
{
	const int mouseY = Mouse_status.y;
	const int mouseX = Mouse_status.x;

	// Position mouse status to the right of HP bar
	constexpr int MOUSE_Y = 0; // Same row as HP bar
	constexpr int MOUSE_X = 35; // After HP bar and values

	mvwprintw(guiWin, MOUSE_Y, MOUSE_X, "Mouse: Y:%d X:%d", mouseY, mouseX);

	if (!ctx.map->is_in_fov(Vector2D{ Mouse_status.y, Mouse_status.x }))
	{
		//if the mouse is out of fov, nothing to render
		return;
	}

	std::string buf;
	bool first = true;

	// Check for creatures at mouse position
	for (const auto& actor : *ctx.creatures)
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

	// Check for items at mouse position
	// Note: Need to update based on actual GameContext inventory structure
	// For now, keeping reference pattern - update based on actual inventory access

	// Display creature and item names to the right of mouse coordinates if any found
	if (!buf.empty())
	{
		mvwprintw(guiWin, MOUSE_Y, MOUSE_X + 20, "[%s]", buf.c_str());
	}
}

void Gui::save(json& j)
{
	//const int logSize = static_cast<int>(log.size());
	//zip.putInt(logSize);
	//for (const auto& message : log)
	//{
	//	zip.putString(message->logMessageText.c_str());
	//	zip.putInt(message->logMessageColor);
	//}
}

void Gui::load(const json& j)
{
	//int nbMessages = zip.getInt();
	//while (nbMessages > 0) 
	//{
	//	const char* text = zip.getString();
	//	const int col = zip.getInt();
	//	log_message(col, text);
	//	nbMessages--;
	//}
}

void Gui::render_hp_bar(const GameContext& ctx)
{
	constexpr int BAR_WIDTH = 20;
	constexpr int BAR_Y = 0;
	constexpr int BAR_X = 1;

	// Get current HP values directly from player
	const int currentHp = ctx.player->destructible->get_hp();
	const int maxHp = ctx.player->destructible->get_max_hp();
	
	if (maxHp <= 0) return; // Prevent division by zero
	
	// Calculate HP percentage and bar width
	const float hpRatio = static_cast<float>(currentHp) / static_cast<float>(maxHp);
	const int filledWidth = static_cast<int>(hpRatio * BAR_WIDTH);
	
	// Determine color based on HP percentage
	int barColor = WHITE_GREEN_PAIR; // Default green (76-100%)
	if (hpRatio <= 0.25f)
	{
		barColor = WHITE_RED_PAIR; // Red for critical HP (0-25%)
	}
	else if (hpRatio <= 0.4f)
	{
		barColor = BLACK_RED_PAIR; // Orange for low HP (26-40%)
	}
	else if (hpRatio <= 0.6f)
	{
		barColor = BLACK_YELLOW_PAIR; // Yellow for medium HP (41-60%)
	}
	else if (hpRatio <= 0.75f)
	{
		barColor = RED_YELLOW_PAIR; // Light orange for good HP (61-75%)
	}
	
	// Print HP bar label
	mvwprintw(guiWin, BAR_Y, BAR_X, "HP:");
	
	// Render filled portion of HP bar
	wattron(guiWin, COLOR_PAIR(barColor));
	for (int i = 0; i < filledWidth; ++i)
	{
		mvwaddch(guiWin, BAR_Y, BAR_X + 3 + i, '#');
	}
	wattroff(guiWin, COLOR_PAIR(barColor));
	
	// Render empty portion of HP bar
	wattron(guiWin, COLOR_PAIR(WHITE_BLACK_PAIR));
	for (int i = filledWidth; i < BAR_WIDTH; ++i)
	{
		mvwaddch(guiWin, BAR_Y, BAR_X + 3 + i, '-');
	}
	wattroff(guiWin, COLOR_PAIR(WHITE_BLACK_PAIR));
	
	// Print HP values at the end of the bar
	mvwprintw(guiWin, BAR_Y, BAR_X + 3 + BAR_WIDTH + 1, "%d/%d", currentHp, maxHp);
}

void Gui::render_hunger_status(const GameContext& ctx)
{
	constexpr int BAR_WIDTH = 20;
	constexpr int BAR_Y = 2; // Position below HP bar
	constexpr int BAR_X = 1;

	if (ctx.hunger_system->get_hunger_max() <= 0) return; // Prevent division by zero

	// Get hunger information
	const int hungerValue = ctx.hunger_system->get_hunger_value();
	const int hungerMax = ctx.hunger_system->get_hunger_max();
	const std::string hungerStateText = ctx.hunger_system->get_hunger_state_string();
	const int hungerColor = ctx.hunger_system->get_hunger_color();
	
	// Calculate hunger percentage and bar width
	const float hungerRatio = static_cast<float>(hungerValue) / static_cast<float>(hungerMax);
	const int filledWidth = static_cast<int>(hungerRatio * BAR_WIDTH);
	
	// Print Hunger bar label
	mvwprintw(guiWin, BAR_Y, BAR_X, "Hunger:");
	
	// Render filled portion of hunger bar
	wattron(guiWin, COLOR_PAIR(hungerColor));
	for (int i = 0; i < filledWidth; ++i)
	{
		mvwaddch(guiWin, BAR_Y, BAR_X + 7 + i, '=');
	}
	wattroff(guiWin, COLOR_PAIR(hungerColor));
	
	// Render empty portion of hunger bar
	wattron(guiWin, COLOR_PAIR(WHITE_BLACK_PAIR));
	for (int i = filledWidth; i < BAR_WIDTH; ++i)
	{
		mvwaddch(guiWin, BAR_Y, BAR_X + 7 + i, '-');
	}
	wattroff(guiWin, COLOR_PAIR(WHITE_BLACK_PAIR));
	
	// Print hunger values and state at the end of the bar
	wattron(guiWin, COLOR_PAIR(hungerColor));
	mvwprintw(guiWin, BAR_Y, BAR_X + 7 + BAR_WIDTH + 1, "%d/%d (%s)", hungerValue, hungerMax, hungerStateText.c_str());
	wattroff(guiWin, COLOR_PAIR(hungerColor));
}

// end of file: Gui.cpp
