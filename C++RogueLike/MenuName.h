// file: MenuName.h
#ifndef MENU_NAME_H
#define MENU_NAME_H

#include <curses.h>
#include <string>

class MenuName
{
	bool run{ true };
	WINDOW* menuNameWindow{ nullptr };

	int menuNameX{ 0 };
	int menuNameY{ 0 };
	int menuNameWidth{ 0 };
	int menuNameHeight{ 0 };

	std::string playerName{ "None" };

	char name[40]; // array to store user input in , 39 is the max length of the name, 40 is the max length + 1 for the null terminator

	void menu_name_new() noexcept;
	void menu_name_clear() noexcept;
	void menu_name_print(int x, int y, const std::string& text) noexcept;
	void menu_name_refresh() noexcept;
	void menu_name_delete() noexcept;

	void menu_name_set_run_true() noexcept { run = true; }
	void menu_name_set_run_false() noexcept { run = false; }

	std::string menu_name_input(); // take user input and return it as a string

	void menu_name_store();
	void menu_name_assign();

public:
	void menu_name();

};

#endif // !MENU_NAME_H
// file: MenuName.h
