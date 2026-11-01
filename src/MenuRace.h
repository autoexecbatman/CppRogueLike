#pragma once

// MenuRace.h - factory for the character creation race selection menu.

#include <memory>

#include "BaseMenu.h"

namespace CppRL
{

	struct GameContext;

	std::unique_ptr<BaseMenu> make_race_menu(GameContext& ctx);

} // namespace CppRL
