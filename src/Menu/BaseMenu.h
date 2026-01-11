#pragma once
#include <curses.h>
#include <string>

struct GameContext;

class BaseMenu
{
protected:
	WINDOW* menuWindow{ nullptr };
	WINDOW* backgroundWindow{ nullptr }; // For background preservation
	int menu_height{ 0 };
	int menu_width{ 0 };
	int menu_starty{ 0 };
	int menu_startx{ 0 };
	int keyPress{ 0 };
	bool needsRedraw{ true }; // Track when redraw is needed
public:
	bool run{ true };
	bool back{ false };
	BaseMenu() = default;
	virtual ~BaseMenu() = default;
	BaseMenu(const BaseMenu&) = delete;
	BaseMenu& operator=(const BaseMenu&) = delete;
	BaseMenu(BaseMenu&&) = delete;
	BaseMenu& operator=(BaseMenu&&) = delete;

	void menu_new(int height, int width, int starty, int startx, GameContext& ctx);
	void menu_clear() 
	{ 
		if (menuWindow) 
		{
			wclear(menuWindow); 
			// Fill with solid background to prevent bleed-through
			wbkgd(menuWindow, ' ' | COLOR_PAIR(0));
		}
		needsRedraw = true; 
	};
	void menu_print(int x, int y, const std::string& text) { mvwprintw(menuWindow, y, x, text.c_str()); };
	void menu_refresh() { wrefresh(menuWindow); };
	void menu_delete();
	void menu_highlight_on() { wattron(menuWindow, A_REVERSE); };
	void menu_highlight_off() { wattroff(menuWindow, A_REVERSE); };
	void menu_key_listen() { keyPress = getch(); };
	void menu_set_run_true() { run = true; };
	void menu_set_run_false() { run = false; };
	void menu_mark_dirty() { needsRedraw = true; }; // Mark for redraw
	bool menu_needs_redraw() const { return needsRedraw; };
	void menu_clear_dirty() { needsRedraw = false; };

	// Web-optimized rendering methods
	void menu_save_background(); // Save screen area behind menu
	void menu_restore_background(); // Restore background when closing
	void menu_draw_box(); // Draw menu box with title
	void menu_efficient_refresh(); // Only refresh if needed

	virtual void menu(GameContext& ctx) = 0;
	virtual void draw_content() {}; // Default empty implementation
};
