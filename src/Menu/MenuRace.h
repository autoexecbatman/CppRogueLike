#pragma once

// MenuRace.h — factory for the character creation race selection menu.

#include <memory>

#include "BaseMenu.h"

struct GameContext;

std::unique_ptr<BaseMenu> make_race_menu(GameContext& ctx);
