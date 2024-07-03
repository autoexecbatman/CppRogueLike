// file: MenuRace.h
#ifndef MENU_RACE_H
#define MENU_RACE_H
#include <curses.h>
#include <string>

class MenuRace
{
public:
	enum class MenuRaceOptions : int
	{
		NONE,
		HUMAN,
		DWARF,
		ELF,
		GNOME,
		HALFELF,
		HALFLING,
		RANDOM,
		BACK
	} currentState{ MenuRaceOptions::HUMAN };
private:
	int oldOption{ -1 };
	int newState{ 1 };

	WINDOW* menuRaceWindow{ nullptr };

	void menu_race_new(int height, int width, int starty, int startx) noexcept { menuRaceWindow = newwin(height, width, starty, startx); }
	void menu_race_clear() noexcept { wclear(menuRaceWindow); }
	void menu_race_print(int x, int y, const std::string& text) noexcept { mvwprintw(menuRaceWindow, y, x, text.c_str()); }
	void menu_race_refresh() noexcept { wrefresh(menuRaceWindow); }
	void menu_race_delete() noexcept { delwin(menuRaceWindow); }

	int menu_race_get_oldOption() noexcept { return oldOption; }
	int menu_race_get_newOption() noexcept { return newState; }
	void menu_race_set_oldOption(int option) noexcept { oldOption = option; }
	void menu_race_set_newOption(int option) noexcept { newState = option; }
	void menu_race_highlight_on() noexcept { wattron(menuRaceWindow, A_REVERSE); }
	void menu_race_highlight_off() noexcept { wattroff(menuRaceWindow, A_REVERSE); }

	std::string menu_race_get_string(MenuRaceOptions option) noexcept;
	void menu_race_print_option(MenuRaceOptions option, int row) noexcept;

	void menu_race_move_up() noexcept { newState--; currentState = static_cast<MenuRaceOptions>(newState); }
	void menu_race_move_down() noexcept { newState++; currentState = static_cast<MenuRaceOptions>(newState); }
	void menu_race_select();

	void menu_race_human();
	void menu_race_dwarf();
	void menu_race_elf();
	void menu_race_gnome();
	void menu_race_halfelf();
	void menu_race_halfling();
	void menu_race_random();

public:
	bool run{ true };

	void menu_race();
	void menu_race_set_run_true() noexcept { run = true; }
	void menu_race_set_run_false() noexcept { run = false; }
};

#endif // !MENU_RACE_H
// end of file: MenuRace.h
