#pragma once

// Menu.h — factory for the main game menu (startup and in-game pause).
// startup=true:  ESC quits the game without saving.
// startup=false: ESC just closes the menu.

#include <memory>

#include "BaseMenu.h"

struct GameContext;

std::unique_ptr<BaseMenu> make_main_menu(bool startup, GameContext& ctx);
