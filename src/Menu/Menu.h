#pragma once

#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

class RoomEditorEntry : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class MonsterEditorEntry : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class SpellEditorEntry : public IMenuState
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
	int menu_height{ 9 };
	int menu_width{ 16 };
	int menu_starty{ 0 };
	int menu_startx{ 0 };
	enum class MenuState
	{
		NEW_GAME,
		LOAD_GAME,
		OPTIONS,
		ROOM_EDITOR,
		MONSTER_EDITOR,
		SPELL_EDITOR,
		QUIT
	} currentState{ MenuState::NEW_GAME };
	std::unordered_map<MenuState, std::unique_ptr<IMenuState>> iMenuStates;
	std::unordered_map<MenuState, std::string> menuStateStrings{
		{ MenuState::NEW_GAME, "New Game" },
		{ MenuState::LOAD_GAME, "Load Game" },
		{ MenuState::OPTIONS, "Options" },
		{ MenuState::ROOM_EDITOR, "Room Editor" },
		{ MenuState::MONSTER_EDITOR, "Monster Editor" },
		{ MenuState::SPELL_EDITOR, "Spell Editor" },
		{ MenuState::QUIT, "Quit" }
	};

	std::string menu_get_string(MenuState state)
	{
		if (!menuStateStrings.contains(state))
		{
			throw std::out_of_range(std::format(
				"Menu::menu_get_string: no label registered for MenuState({})",
				static_cast<int>(state)));
		}
		return menuStateStrings.at(state);
	}
	void menu_print_state(MenuState state);
	void draw_content() override;

public:
	Menu(bool startup, GameContext& ctx);
	~Menu();

	void draw();
	void on_key(int key, GameContext& ctx);
	void menu(GameContext& ctx) override;
};
