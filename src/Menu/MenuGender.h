#pragma once

// MenuGender.h — factory for the character creation gender selection menu.

#include <memory>

#include "BaseMenu.h"

struct GameContext;

std::unique_ptr<BaseMenu> make_gender_menu(GameContext& ctx);
