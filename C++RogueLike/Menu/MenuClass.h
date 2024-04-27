// file: MenuClass.h
#ifndef MENU_CLASS_H
#define MENU_CLASS_H
#include <curses.h>
#include <string>

class MenuClass
{
public:
	enum class MenuClassOptions : int
	{
		NONE,
		FIGHTER,
		ROGUE,
		CLERIC,
		WIZARD,
		RANDOM,
		BACK
	} currentOption{ MenuClassOptions::FIGHTER };
private:
	int oldOption{ -1 };
	int newOption{ 1 };

	WINDOW* menuClassWindow{ nullptr };

	void menu_class_new(int height, int width, int starty, int startx) noexcept { menuClassWindow = newwin(height, width, starty, startx); }
	void menu_class_clear() noexcept { wclear(menuClassWindow); }
	void menu_class_print(int x, int y, const std::string& text) noexcept { mvwprintw(menuClassWindow, y, x, text.c_str()); }
	void menu_class_refresh() noexcept { wrefresh(menuClassWindow); }
	void menu_class_delete() noexcept { delwin(menuClassWindow); }
	void menu_class_set_run_true() noexcept { run = true; }
	void menu_class_set_run_false() noexcept { run = false; }
	int menu_class_get_oldOption() noexcept { return oldOption; }
	int menu_class_get_newOption() noexcept { return newOption; }
	void menu_class_set_oldOption(int option) noexcept { oldOption = option; }
	void menu_class_set_newOption(int option) noexcept { newOption = option; }
	void menu_class_highlight_on() noexcept { wattron(menuClassWindow, A_REVERSE); }
	void menu_class_highlight_off() noexcept { wattroff(menuClassWindow, A_REVERSE); }

	std::string menu_class_get_string(MenuClassOptions option) noexcept;
	void menu_class_print_option(MenuClassOptions option, int row) noexcept;

	void menu_class_move_up() noexcept { newOption--; currentOption = static_cast<MenuClassOptions>(newOption); }
	void menu_class_move_down() noexcept { newOption++; currentOption = static_cast<MenuClassOptions>(newOption); }
	void menu_class_select();

	// create function for fighter, rogue, cleric, wizard, random, back
	void menu_class_fighter();
	void menu_class_rogue();
	void menu_class_cleric();
	void menu_class_wizard();
	void menu_class_random();

public:
	bool run{ true };

	void menu_class();
};

#endif // !MENU_CLASS_H
// end of file: MenuClass.h
