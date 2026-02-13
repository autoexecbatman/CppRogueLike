#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "BaseMenu.h"
#include "IMenuState.h"

struct GameContext;

class NewGame : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class LoadGame : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Options : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Quit : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Menu : public BaseMenu
{
	bool isStartupMenu{ true }; // Track if this is the startup menu (default true)
	int menu_height{ 6 }; // Height for the menu, enough for 4 options + title
	int menu_width{ 11 }; // Wide enough for "Load Game"
	int menu_starty{ (30 - menu_height) / 2 };
	int menu_startx{ (119 - menu_width) / 2 };
	enum class MenuState { NEW_GAME, LOAD_GAME, OPTIONS, QUIT }
	currentState { MenuState::NEW_GAME };
	std::unordered_map<MenuState, std::unique_ptr<IMenuState>> iMenuStates;
	std::unordered_map<MenuState, std::string> menuStateStrings
	{
		{ MenuState::NEW_GAME, "New Game" },
		{ MenuState::LOAD_GAME, "Load Game" },
		{ MenuState::OPTIONS, "Options" },
		{ MenuState::QUIT, "Quit" }
	};

	std::string menu_get_string(MenuState state) { return menuStateStrings.at(state); }
	void menu_print_state(MenuState state);
	void draw_content() override;
public:
	Menu(bool startup, GameContext& ctx);
	~Menu();

	void draw();
	void on_key(int key, GameContext& ctx);
	void menu(GameContext& ctx) override;
};
