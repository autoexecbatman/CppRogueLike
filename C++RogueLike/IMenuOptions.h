#pragma once

#include <iostream>

#include "Game.h"
#include "Menu/Menu.h"
#include "Menu/MenuGender.h"


class IMenuOptions
{
public:
	virtual void on_selection() = 0;
	virtual ~IMenuOptions() = default;
};

class NewGame : public IMenuOptions
{
public:
	void on_selection() override;

};

class LoadGame : public IMenuOptions
{
	void on_selection() override
	{
		std::cout << "Load Game Selected" << std::endl;
	}
};

class Options : public IMenuOptions
{
	void on_selection() override
	{
		std::cout << "Options Selected" << std::endl;
	}
};

class Quit : public IMenuOptions
{
	void on_selection() override
	{
		std::cout << "Quit Selected" << std::endl;
	}
};