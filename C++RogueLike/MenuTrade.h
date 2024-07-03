#pragma once

#include <curses.h>
#include <vector>
#include "Menu/Menu.h"

class MenuTrade : public BaseMenu
{
private:
	bool run{ true };
	int menu_height{ 0 };
	int menu_width{ 0 };
	int menu_starty{ 0 };
	int menu_startx{ 0 };
	int keyPress{ 0 };
	int currentState{ 0 };
	int stateInt{ 0 };
	void key_listen() { keyPress = wgetch(menuWindow); }

	void print_option(int x, int y, const std::string& text)
	{
		if (currentState == y - 1)
		{
			wattron(menuWindow, A_REVERSE);
			mvwprintw(menuWindow, y, x, text.c_str());
			wattroff(menuWindow, A_REVERSE);
		}
		else
		{
			mvwprintw(menuWindow, y, x, text.c_str());
		}
	}

	std::vector<std::string_view> options;
public:
	MenuTrade(int height, int width, int starty, int startx)
	{
		menuWindow = newwin(menu_height, menu_width, menu_starty, menu_startx);
	}

	void menu()
	{
		while (run)
		{
			// clear the screen each frame
			wclear(menuWindow);

			// print N number of options each in a new line
			for (size_t i = 0; i < options.size(); i++)
			{
				print_option(1, i + 1, options.at(i).data());
			}
			
			wrefresh(menuWindow);// refresh menu window each frame to show changes

			key_listen(); // listen for key presses
			switch (keyPress)
			{
				case KEY_UP:
				{
					currentState--; // decrement newOption
					stateInt = currentState; // set currentOption to newOption
					break; // break out of switch keep running menu loop
				}

				case KEY_DOWN:
				{
					currentState++; // increment newOption
					stateInt = currentState; // set currentOption to newOption
					break; // break out of switch keep running menu loop
				}

				case 10: // Enter key
				{
					switch (currentState)
					{
						// return result
						case 0:
							run = false;
							break;
					}
				}
				case 'Q':
				case 'q':
				{
					run = false; // stop running menu loop
					break; // break out of switch keep running menu loop
				}

				default:
					break;
			}

			box(menuWindow, 0, 0);
			wrefresh(menuWindow);
		}
	}

	void populate_map(std::string_view text) { options.emplace_back(text); }
	/*void execute_option() { options.at(currentOption); }*/
};
