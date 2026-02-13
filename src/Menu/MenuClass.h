#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "BaseMenu.h"
#include "IMenuState.h"

class Fighter : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Rogue : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Cleric : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Wizard : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class ClassRandom : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class ClassBack : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class MenuClass : public BaseMenu
{
	int menu_height{ 10 };
	int menu_width{ 14 };
	int menu_starty{ (30 - menu_height) / 2 };
	int menu_startx{ (119 - menu_width) / 2 };
	enum class MenuState { FIGHTER, ROGUE, CLERIC, WIZARD, RANDOM, BACK }
	currentState{ MenuState::FIGHTER };
	std::unordered_map<MenuState, std::unique_ptr<IMenuState>> iMenuStates;
	std::unordered_map<MenuState, std::string> menuClassStrings
	{
		{ MenuState::FIGHTER, "Fighter" },
		{ MenuState::ROGUE, "Rogue" },
		{ MenuState::CLERIC, "Cleric" },
		{ MenuState::WIZARD, "Wizard" },
		{ MenuState::RANDOM, "Random" },
		{ MenuState::BACK, "Back" }
	};

	std::string menu_class_get_string(MenuState option) { return menuClassStrings.at(option); };
	void menu_class_print_option(MenuState option) noexcept;
public:
	MenuClass(GameContext& ctx);
	~MenuClass();

	void draw();
	void on_key(int key, GameContext& ctx);
	void menu(GameContext& ctx) override;
};
