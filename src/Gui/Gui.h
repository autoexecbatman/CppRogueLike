#pragma once

#include <curses.h>
#include <vector>
#include <string>

#include "../Persistent/Persistent.h"
#include "LogMessage.h"

// Forward declaration
struct GameContext;

inline constexpr int GUI_HEIGHT{ 7 };
inline int gui_width() { return COLS - 1; }

// Description:
// The Gui class is responsible for displaying the gui window
// and all of the information that is displayed in the gui window.
//
// Functions:
// The constructor and destructor are empty
// because the gui window is initialized and deleted
// using explicit calls to the gui_init() and gui_delete() functions.
//
class Gui : public Persistent
{
private:
	int guiMessageColor{ 0 }; // the color of the message to be displayed on the gui
	std::string guiMessage{}; // the message to be displayed on the gui
	std::vector<std::vector<LogMessage>> displayMessages; // a container to read messages from, having a color and a string as a pair

	WINDOW* guiWin{ nullptr };

	void gui_new(int height, int width, int starty, int startx) noexcept { guiWin = newwin(height, width, starty, startx); }
	void gui_clear() noexcept { wclear(guiWin); }
	void gui_print(int x, int y, const std::string& text) noexcept { mvwprintw(guiWin, y, x, text.c_str()); }
	void gui_refresh() noexcept { wrefresh(guiWin); }
	void gui_delete() noexcept { delwin(guiWin); }

public:
	bool guiInit{ false };

	void gui_init() noexcept; // initialize the gui
	void gui_shutdown() noexcept; // shutdown the gui
	void gui_update(GameContext& ctx); // update the gui
	void gui_render(const GameContext& ctx); // render the gui

	void render_player_status(const GameContext& ctx);

	void gui_print_stats(const GameContext& ctx, std::string_view playerName, int guiHp, int guiHpMax, std::string_view roll, int dr) noexcept;
	void gui_print_log(const GameContext& ctx);
	void gui_print_attrs(int str, int dex, int con, int inte, int wis, int cha) noexcept;

	void gui(GameContext& ctx) noexcept;
	
	void load(const json& j) override;
	void save(json& j) override;

	void add_display_message(const std::vector<LogMessage>& message);
	void render_messages() noexcept;

	// setters and getters
	void set_message(const std::string& msg) { guiMessage = msg; }
	void set_message_color(int color) { guiMessageColor = color; }
	const std::string& get_message() const { return guiMessage; }
	int get_message_color() const { return guiMessageColor; }

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

	void renderMouseLook(const GameContext& ctx);
	void render_hp_bar(const GameContext& ctx);
	void render_hunger_status(const GameContext& ctx);
};
