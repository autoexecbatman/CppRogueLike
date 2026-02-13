#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "BaseMenu.h"
#include "IMenuState.h"

class IMenuState;
struct GameContext;

class Male : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Female : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Random : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class Back : public IMenuState
{
	void on_selection(GameContext& ctx) override;
};

class MenuGender : public BaseMenu
{
	int height_{ 6 };
	int width_{ 15 };
	int starty_{ (30 - height_) / 2 };
	int startx_{ (119 - width_) / 2 };
	enum class MenuState { MALE, FEMALE, RANDOM, BACK }
	currentState{ MenuState::MALE };
	std::unordered_map<MenuState, std::unique_ptr<IMenuState>> iMenuStates;
	std::unordered_map<MenuState, std::string> menuStateStrings
	{
		{ MenuState::MALE, "Male"},
		{ MenuState::FEMALE, "Female"},
		{ MenuState::RANDOM, "Random"},
		{ MenuState::BACK, "Back"}
	};

	std::string menu_gender_get_string(MenuState option) { return menuStateStrings.at(option); }
	void menu_print_state(MenuState option);
public:
	MenuGender(GameContext& ctx);
	~MenuGender();

	void draw();
	void on_key(int key, GameContext& ctx);
	void menu(GameContext& ctx) override;
};
