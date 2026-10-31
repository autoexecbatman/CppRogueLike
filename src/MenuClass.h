#pragma once

// MenuClass.h - factory for the character creation class selection menu.

#include <memory>

#include "BaseMenu.h"

namespace CppRL
{

struct GameContext;

std::unique_ptr<BaseMenu> make_class_menu(GameContext& ctx);

} // namespace CppRL
