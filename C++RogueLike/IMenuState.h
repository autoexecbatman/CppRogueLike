#pragma once

#include <iostream>

#include "Game.h"
#include "Menu/Menu.h"
#include "Menu/MenuGender.h"


class IMenuState
{
public:
	virtual void on_selection() = 0;
	virtual ~IMenuState() = default;
};
